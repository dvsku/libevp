<div align="center">
  <h1>libevp</h1>
  <p>
    <span>
      <img src="https://img.shields.io/github/actions/workflow/status/dvsku/libevp/build-windows.yml?branch=main&label=windows%20build"/>
    </span>
    <span>
      <img src="https://img.shields.io/github/actions/workflow/status/dvsku/libevp/build-linux.yml?branch=main&label=linux%20build"/>
    </span>
    <span>
      <img src="https://img.shields.io/github/downloads/dvsku/libevp/total"/>
    </span>
    <span>
      <img src="https://img.shields.io/github/license/dvsku/libevp"/>
    </span>
  </p>
  <p>
    Library for packing/unpacking Talisman Online .evp files
  </p>
</div>
</br>

## Legend
### Format v1
EVP internal type: 100/0x64 \
0x64 usage: present in client v207 (used on all private servers)

### Format v2
EVP internal type: 101/0x65 and 102/0x66 \
0x65 usage: present in client v2013 (used for dev) \
0x66 usage: present in client v5925 (used for dev)

Based on <a href="https://aluigi.altervista.org/quickbms.htm">Luigi Auriemma's QuickBMS script</a> with some minor fixes that correctly unpack files that the script fails to.

## Support
### Packing
- Format v1

### Unpacking
- Format v1
- Format v2

## Requirements
- c++20

## Limitations
### File validation
Format v2 file validation fails due to model/texture/scene files being further encrypted. \
Handling of that encryption is out of scope for this lib.
