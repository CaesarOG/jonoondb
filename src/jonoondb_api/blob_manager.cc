#include "blob_manager.h"
#include <assert.h>
#include <boost/endian/conversion.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include "blob_metadata.h"
#include "buffer_impl.h"
#include "exception_utils.h"
#include "file.h"
#include "filename_manager.h"
#include "jonoondb_utils/varint.h"
#include "lz4.h"
#include "standard_deleters.h"

using namespace std;
using namespace boost::filesystem;
using namespace jonoondb_api;
using namespace jonoondb_utils;

#define DEFAULT_MEM_MAP_LRU_CACHE_SIZE 3

namespace jonoondb_api {
const uint8_t kBlobHeaderVersion = 1;

struct BlobHeader {
  std::uint8_t version;
  bool compressed;
  std::uint64_t blobSize;
  std::uint64_t compSize;

  inline static int GetVarintSize(std::uint64_t num) {
    if (num < 128) {
      return 1;
    } else if (num < 16384) {
      return 2;
    } else if (num < 2097152) {
      return 3;
    } else if (num < 268435456) {
      return 4;
    } else if (num < 34359738368U) {
      return 5;
    } else if (num < 4398046511104U) {
      return 6;
    } else if (num < 562949953421312U) {
      return 7;
    } else if (num < 72057594037927936U) {
      return 8;
    } else if (num < 9223372036854775808U) {
      return 9;
    } else {
      return 10;
    }
  }

  inline static int GetHeaderSize(std::uint64_t blobSize, int compBlobSize) {
    auto num1 = GetVarintSize(blobSize);
    auto num2 = 0;
    if (compBlobSize > -1) {
      num2 = GetVarintSize(compBlobSize);
    }

    return num1 + num2 + 1;  // 1 is the fixed size for verAndFlags
  }

  inline static void ReadBlobHeader(char*& offsetAddress, BlobHeader& header) {
    // Header: VerAndFlags (1 Byte) + SizeOfBlob (varint)
    //         + CompressedBlobSize [only if compressed] (varint)
    std::uint8_t verAndFlags = 0;
    memcpy(&verAndFlags, offsetAddress, 1);
    offsetAddress++;
    // Drop last 4 bits
    header.version = verAndFlags & 0xF0;  // 0xF0 is equal to 1111 0000
    header.version = header.version >> 4;

    header.compressed = (verAndFlags & 1) == 1;

    auto varIntSize =
        Varint::DecodeVarint((std::uint8_t*)*&offsetAddress, &header.blobSize);
    if (varIntSize == -1) {
      std::string msg =
          "Failed to read the blob header. Varint blobSize is greater than 10 "
          "bytes.";
      throw JonoonDBException(msg, __FILE__, __func__, __LINE__);
    }
    offsetAddress += varIntSize;

    if (header.compressed) {
      varIntSize = Varint::DecodeVarint((std::uint8_t*)*&offsetAddress,
                                        &header.compSize);
      if (varIntSize == -1) {
        std::string msg =
            "Failed to read the blob header. Varint compBlobSize is greater "
            "than 10 bytes.";
        throw JonoonDBException(msg, __FILE__, __func__, __LINE__);
      }
      offsetAddress += varIntSize;
    }
  }

  inline static int WriteBlobHeader(
      std::shared_ptr<MemoryMappedFile>& memMappedFile,
      const BlobHeader& header) {
    // Write the header
    // Header: VerAndFlags (1 Byte) + SizeOfBlob (varint)
    //         + CompressedBlobSize [only if compressed] (varint)
    std::uint8_t verAndFlags = 0;
    verAndFlags |= 1 << 4;                     // version
    verAndFlags |= header.compressed ? 1 : 0;  // compression flag

    memMappedFile->WriteAtCurrentPosition(&verAndFlags, sizeof(verAndFlags));

    std::uint8_t varIntBuffer[kMaxVarintBytes];
    auto varintSize = Varint::EncodeVarint(header.blobSize, varIntBuffer);
    int varintSum = varintSize;
    memMappedFile->WriteAtCurrentPosition(&varIntBuffer, varintSize);

    if (header.compressed) {
      varintSize = Varint::EncodeVarint(header.compSize, varIntBuffer);
      varintSum += varintSize;
      memMappedFile->WriteAtCurrentPosition(&varIntBuffer, varintSize);
    }

    // return bytes written
    return sizeof(verAndFlags) + varintSum;
  }
};
}  // namespace jonoondb_api

int GetCompressedSize(std::uint64_t size) {
  if (size > (std::uint64_t)LZ4_MAX_INPUT_SIZE) {
    std::ostringstream ss;
    ss << "Unable to compress data of size " << size
       << ". Its greater than LZ4_MAX_INPUT_SIZE i.e. " << LZ4_MAX_INPUT_SIZE
       << ".";
    throw JonoonDBException(ss.str(), __FILE__, __func__, __LINE__);
  }

  return LZ4_compressBound(static_cast<int>(size));
}

BlobManager::BlobManager(unique_ptr<FileNameManager> fileNameManager,
                         size_t maxDataFileSize, bool synchronous)
    : m_fileNameManager(move(fileNameManager)),
      m_maxDataFileSize(maxDataFileSize),
      m_currentBlobFile(nullptr),
      m_synchronous(synchronous),
      m_readerFiles(DEFAULT_MEM_MAP_LRU_CACHE_SIZE) {
  m_fileNameManager->GetCurrentDataFileInfo(true, m_currentBlobFileInfo);
  path pathObj(m_currentBlobFileInfo.fileNameWithPath);
  // Check if the file exist or do we have to create it
  if (!boost::filesystem::exists(pathObj)) {
    File::FastAllocate(m_currentBlobFileInfo.fileNameWithPath,
                       m_maxDataFileSize);
  }
  // We have the file lets memory map it
  m_currentBlobFile.reset(
      new MemoryMappedFile(m_currentBlobFileInfo.fileNameWithPath,
                           MemoryMappedFileMode::ReadWrite, 0, !m_synchronous));

  // Set the MemMapFile offset
  if (m_currentBlobFileInfo.dataLength != -1) {
    m_currentBlobFile->SetCurrentWriteOffset(m_currentBlobFileInfo.dataLength);
    // Reset the data length of the blob file, we will set it on file switch or
    // shutdown
    m_fileNameManager->UpdateDataFileLength(
        m_currentBlobFileInfo.fileKey,
        m_currentBlobFile->GetCurrentWriteOffset());
  }

  m_readerFiles.Add(m_currentBlobFileInfo.fileKey, m_currentBlobFile, false);
}

void BlobManager::Put(const BufferImpl& blob, BlobMetadata& blobMetadata,
                      bool compress) {
  // Lock will be acquired on the next line and released when lock goes out of
  // scope
  lock_guard<mutex> lock(m_writeMutex);
  size_t currentOffsetInFile = m_currentBlobFile->GetCurrentWriteOffset();
  int compSize = compress ? GetCompressedSize(blob.GetLength()) : -1;
  int headerSize = BlobHeader::GetHeaderSize(blob.GetLength(), compSize);
  auto estimatedBytesToWrite =
      headerSize + (compress ? compSize : blob.GetLength());

  if (estimatedBytesToWrite + currentOffsetInFile > m_maxDataFileSize) {
    SwitchToNewDataFile();
    currentOffsetInFile = m_currentBlobFile->GetCurrentWriteOffset();
  }

  try {
    size_t bytesWritten = PutInternal(blob, blobMetadata, compress);
    // Flush the contents to ensure durability
    Flush(currentOffsetInFile, bytesWritten);
  } catch (...) {
    m_currentBlobFile->SetCurrentWriteOffset(currentOffsetInFile);
    throw;
  }

  // Set the file length
  m_fileNameManager->UpdateDataFileLength(
      m_currentBlobFileInfo.fileKey,
      m_currentBlobFile->GetCurrentWriteOffset());
}

void BlobManager::MultiPut(gsl::span<const BufferImpl*> blobs,
                           std::vector<BlobMetadata>& blobMetadataVec,
                           bool compress) {
  assert(blobs.size() == blobMetadataVec.size());
  size_t bytesWritten = 0, totalBytesWrittenInFile = 0;
  // Lock will be acquired on the next line and released when lock goes out of
  // scope
  lock_guard<mutex> lock(m_writeMutex);
  size_t baseOffsetInFile = m_currentBlobFile->GetCurrentWriteOffset();

  for (int i = 0; i < blobs.size(); i++) {
    size_t currentOffset = m_currentBlobFile->GetCurrentWriteOffset();
    int compSize = compress ? GetCompressedSize(blobs[i]->GetLength()) : -1;
    int headerSize = BlobHeader::GetHeaderSize(blobs[i]->GetLength(), compSize);
    auto estimatedBytesToWrite =
        headerSize + (compress ? compSize : blobs[i]->GetLength());
    if (estimatedBytesToWrite + currentOffset > m_maxDataFileSize) {
      // The file size will exceed the m_maxDataFileSize if blob is written in
      // the current file First flush the contents if required
      try {
        Flush(baseOffsetInFile, totalBytesWrittenInFile);
      } catch (...) {
        m_currentBlobFile->SetCurrentWriteOffset(baseOffsetInFile);
        throw;
      }
      // Now lets switch to a new file
      SwitchToNewDataFile();

      // Reset baseOffset and totalBytesWritten
      baseOffsetInFile = m_currentBlobFile->GetCurrentWriteOffset();
      totalBytesWrittenInFile = 0;
    }

    try {
      bytesWritten = PutInternal(*blobs[i], blobMetadataVec[i], compress);
    } catch (...) {
      m_currentBlobFile->SetCurrentWriteOffset(baseOffsetInFile);
      throw;
    }

    totalBytesWrittenInFile += bytesWritten;
  }

  // Flush to make sure all blobs are written to disk
  try {
    Flush(baseOffsetInFile, totalBytesWrittenInFile);
  } catch (...) {
    m_currentBlobFile->SetCurrentWriteOffset(baseOffsetInFile);
    throw;
  }

  // Set the file length
  m_fileNameManager->UpdateDataFileLength(
      m_currentBlobFileInfo.fileKey,
      m_currentBlobFile->GetCurrentWriteOffset());
}

void BlobManager::Get(const BlobMetadata& blobMetaData, BufferImpl& blob) {
  // Get the FileInfo
  auto fileInfo = make_shared<FileInfo>();
  m_fileNameManager->GetFileInfo(blobMetaData.fileKey, fileInfo);

  // Get the file to read the data from
  std::shared_ptr<MemoryMappedFile> memMapFile;
  if (!m_readerFiles.Find(fileInfo->fileKey, memMapFile)) {
    // Open the memmap file
    memMapFile.reset(new MemoryMappedFile(fileInfo->fileNameWithPath.c_str(),
                                          MemoryMappedFileMode::ReadOnly, 0,
                                          !m_synchronous));

    // Add mmap file in the ConcurrentMap for memmap file for future use
    m_readerFiles.Add(fileInfo->fileKey, memMapFile, true);
  }

  // Read the data from the file
  char* offsetAddress =
      memMapFile->GetOffsetAddressAsCharPtr(blobMetaData.offset);

  // Now read the header.
  BlobHeader header;
  BlobHeader::ReadBlobHeader(offsetAddress, header);
  if (blob.GetCapacity() < header.blobSize) {
    // Passed in buffer is not big enough. Lets resize it
    blob.Resize(header.blobSize);
  }

  // Read Blob contents
  if (header.compressed) {
    // Decompress the data
    int val = LZ4_decompress_fast(offsetAddress, blob.GetDataForWrite(),
                                  header.blobSize);
    if (val < 0) {
      std::ostringstream ss;
      ss << "Decompression failed while reading blob from file "
         << memMapFile->GetFileName() << " at offset " << blobMetaData.offset
         << ". Error code returned by compression lib " << val << ".";
    }
    blob.SetLength(header.blobSize);
  } else {
    blob.Copy(offsetAddress, header.blobSize);
  }
}

void BlobManager::UnmapLRUDataFiles() {
  m_readerFiles.PerformEviction();
}

BlobIterator::BlobIterator(FileInfo fileInfo)
    : m_fileInfo(std::move(fileInfo)),
      m_memMapFile(m_fileInfo.fileNameWithPath, MemoryMappedFileMode::ReadOnly,
                   0, true),
      m_currentOffsetAddress(m_memMapFile.GetOffsetAddressAsCharPtr(0)) {}

std::size_t BlobIterator::GetNextBatch(
    std::vector<BufferImpl>& blobs,
    std::vector<BlobMetadata>& blobMetadataVec) {
  assert(blobs.size() == blobMetadataVec.size());
  assert(blobs.size() > 0);

  std::size_t batchSize = 0;

  for (size_t i = 0; i < blobs.size(); i++) {
    auto position = m_currentOffsetAddress -
                    static_cast<char*>(m_memMapFile.GetBaseAddress());
    if (position >= m_fileInfo.dataLength) {
      // We are at the end of file
      break;
    }
    // Now read the header.
    BlobHeader header;
    BlobHeader::ReadBlobHeader(m_currentOffsetAddress, header);

    if (header.compressed) {
      if (blobs[i].GetCapacity() < header.blobSize) {
        // Passed in buffer is not big enough.
        // Lets resize it to 2x, these buffers
        // are reused again and it will reduce the amount of
        // total memory allocations.
        blobs[i].Resize((header.blobSize) * 2);
      }
      // Decompress the data
      int val = LZ4_decompress_fast(
          m_currentOffsetAddress, blobs[i].GetDataForWrite(), header.blobSize);
      if (val < 0) {
        std::ostringstream ss;
        ss << "Decompression failed while reading blob from file "
           << m_fileInfo.fileNameWithPath << " at offset " << position
           << ". Error code returned by compression lib " << val << ".";
      }
      blobs[i].SetLength(header.blobSize);
      m_currentOffsetAddress += header.compSize;
    } else {
      blobs[i] = std::move(BufferImpl(m_currentOffsetAddress, header.blobSize,
                                      header.blobSize, StandardDeleteNoOp));
      m_currentOffsetAddress += header.blobSize;
    }

    blobMetadataVec[i].fileKey = m_fileInfo.fileKey;
    blobMetadataVec[i].offset = position;
    ++batchSize;
  }

  return batchSize;
}

inline void BlobManager::Flush(size_t offset, size_t numBytes) {
  m_currentBlobFile->Flush(offset, numBytes);
}

void BlobManager::SwitchToNewDataFile() {
  FileInfo fileInfo;
  m_fileNameManager->GetNextDataFileInfo(fileInfo);
  File::FastAllocate(fileInfo.fileNameWithPath, m_maxDataFileSize);

  auto file = std::make_unique<MemoryMappedFile>(
      fileInfo.fileNameWithPath, MemoryMappedFileMode::ReadWrite, 0,
      !m_synchronous);
  m_fileNameManager->UpdateDataFileLength(
      m_currentBlobFileInfo.fileKey,
      m_currentBlobFile->GetCurrentWriteOffset());

  // Set the evictable flag on the current file before switching
  bool retVal = m_readerFiles.SetEvictable(m_currentBlobFileInfo.fileKey, true);
  assert(retVal);

  m_currentBlobFileInfo = fileInfo;
  m_currentBlobFile.reset(file.release());
  m_readerFiles.Add(m_currentBlobFileInfo.fileKey, m_currentBlobFile, false);
}

size_t BlobManager::PutInternal(const BufferImpl& blob,
                                BlobMetadata& blobMetadata, bool compress) {
  BlobHeader header;
  header.version = kBlobHeaderVersion;
  header.compressed = compress;
  size_t bytesWritten = 0;
  // Record the current offset.
  size_t offset = m_currentBlobFile->GetCurrentWriteOffset();
  // Compress if required and capture the storage size of blob
  if (compress) {
    auto maxCompSize = GetCompressedSize(blob.GetLength());
    if (maxCompSize > m_compBuffer.GetCapacity()) {
      m_compBuffer.Resize(maxCompSize);
    }
    auto compSize =
        LZ4_compress_default(blob.GetData(), m_compBuffer.GetDataForWrite(),
                             blob.GetLength(), m_compBuffer.GetCapacity());
    if (compSize == 0) {
      // throw
    }
    m_compBuffer.SetLength(compSize);
    header.compSize = compSize;
    header.blobSize = blob.GetLength();
    // Write the header
    auto headerBytes = BlobHeader::WriteBlobHeader(m_currentBlobFile, header);
    // Write the blob contents
    m_currentBlobFile->WriteAtCurrentPosition(m_compBuffer.GetData(),
                                              m_compBuffer.GetLength());
    bytesWritten = headerBytes + m_compBuffer.GetLength();
  } else {
    header.blobSize = blob.GetLength();
    // Write the header
    auto headerBytes = BlobHeader::WriteBlobHeader(m_currentBlobFile, header);
    // Write the blob contents
    m_currentBlobFile->WriteAtCurrentPosition(blob.GetData(), blob.GetLength());
    bytesWritten = headerBytes + blob.GetLength();
  }

  // 6. Fill and return blobMetaData
  blobMetadata.offset = offset;
  blobMetadata.fileKey = m_currentBlobFileInfo.fileKey;

  return bytesWritten;
}
