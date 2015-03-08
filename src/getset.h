
#ifndef _GETSET_H
#define _GETSET_H

#define GSET_UINT8(_ptr, _offset, _data) \
  *((unsigned char*) (((unsigned char*)(_ptr)) + (_offset))) = (unsigned char)(_data)
#define GGET_UINT8(_ptr, _offset) \
  (*((unsigned char*) (((unsigned char*)(_ptr)) + (_offset))))
#define GSET_UINT16(_ptr, _offset, _data) \
  GSET_UINT8(_ptr, _offset, _data); \
  GSET_UINT8(_ptr, (_offset) + 1, (_data) >> 8)
#define GGET_UINT16(_ptr, _offset) \
  (GGET_UINT8(_ptr, _offset)) | \
  ((GGET_UINT8(_ptr, (_offset) + 1)) << 8)
#define GSET_UINT32(_ptr, _offset, _data) \
  GSET_UINT16(_ptr, _offset, _data); \
  GSET_UINT16(_ptr, (_offset) + 2, (_data) >> 16)
#define GGET_UINT32(_ptr, _offset) \
  (GGET_UINT16(_ptr, _offset)) | \
  ((GGET_UINT16(_ptr, (_offset) + 2)) << 16)

#define GASET_UINT32(_ptr, _offset, _data) \
  ((unsigned int*)(_ptr))[(_offset) / 4] = _data
#define GAGET_UINT32(_ptr, _offset) \
  ((unsigned int*)(_ptr))[(_offset) / 4]

#endif

