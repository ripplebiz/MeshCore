#if defined(PLATFORM_NATIVE)
#include "NativeFS.h"
#define File NativeFS::File
#endif
#include "IdentityStore.h"

bool IdentityStore::load(const char *name, mesh::LocalIdentity& id) {
  bool loaded = false;
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);
  if (_fs->exists(filename)) {
#if defined(RP2040_PLATFORM)
    File file = _fs->open(filename, "r");
#elif defined(PLATFORM_NATIVE)
    File file = _fs->open(filename);
    if (file) {
      uint8_t buf[128];
      size_t n = file.read(buf, sizeof(buf));
      if (n > 0) {
        id.readFrom(buf, n);
        loaded = true;
      }
      file.close();
    }
    return loaded;
#else
    File file = _fs->open(filename);
#endif
    if (file) {
#if !defined(PLATFORM_NATIVE)
      loaded = id.readFrom(file);
#endif
      file.close();
    }
  }
  return loaded;
}

bool IdentityStore::load(const char *name, mesh::LocalIdentity& id, char display_name[], int max_name_sz) {
  bool loaded = false;
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);
  if (_fs->exists(filename)) {
#if defined(RP2040_PLATFORM)
    File file = _fs->open(filename, "r");
#elif defined(PLATFORM_NATIVE)
    File file = _fs->open(filename);
    if (file) {
      uint8_t buf[128];
      size_t n = file.read(buf, sizeof(buf));
      if (n > 0) {
        id.readFrom(buf, n);
        int copy_n = max_name_sz;
        if (copy_n > 32) copy_n = 32;
        memcpy(display_name, buf + sizeof(mesh::LocalIdentity), copy_n);
        display_name[copy_n - 1] = 0;
        loaded = true;
      }
      file.close();
    }
    return loaded;
#else
    File file = _fs->open(filename);
#endif
    if (file) {
#if !defined(PLATFORM_NATIVE)
      loaded = id.readFrom(file);
      int n = max_name_sz;   // up to 32 bytes
      if (n > 32) n = 32;
      file.read((uint8_t *) display_name, n);
      display_name[n - 1] = 0;  // ensure null terminator
#endif
      file.close();
    }
  }
  return loaded;
}

bool IdentityStore::save(const char *name, const mesh::LocalIdentity& id) {
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  _fs->remove(filename);
  File file = _fs->open(filename, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = _fs->open(filename, "w");
#elif defined(PLATFORM_NATIVE)
  File file = _fs->open(filename, "w", true);
  if (file) {
    uint8_t buf[128];
    mesh::LocalIdentity temp_id = id;  // Create a non-const copy
    size_t n = temp_id.writeTo(buf, sizeof(buf));
    if (n > 0) {
      file.write(buf, n);
      file.close();
      MESH_DEBUG_PRINTLN("IdentityStore::save() write - OK");
      return true;
    }
    file.close();
  }
  MESH_DEBUG_PRINTLN("IdentityStore::save() failed");
  return false;
#else
  File file = _fs->open(filename, "w", true);
#endif
  if (file) {
#if !defined(PLATFORM_NATIVE)
    mesh::LocalIdentity temp_id = id;  // Create a non-const copy
    bool success = temp_id.writeTo(file);
    file.close();
    MESH_DEBUG_PRINTLN("IdentityStore::save() write - %s", success ? "OK" : "Err");
    return success;
#endif
    file.close();
  }
  MESH_DEBUG_PRINTLN("IdentityStore::save() failed");
  return false;
}

bool IdentityStore::save(const char *name, const mesh::LocalIdentity& id, const char display_name[]) {
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  _fs->remove(filename);
  File file = _fs->open(filename, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = _fs->open(filename, "w");
#elif defined(PLATFORM_NATIVE)
  File file = _fs->open(filename, "w", true);
  if (file) {
    uint8_t buf[128];
    mesh::LocalIdentity temp_id = id;  // Create a non-const copy
    size_t n = temp_id.writeTo(buf, sizeof(buf));
    if (n > 0) {
      file.write(buf, n);
      uint8_t tmp[32];
      memset(tmp, 0, sizeof(tmp));
      int copy_n = strlen(display_name);
      if (copy_n > sizeof(tmp)-1) copy_n = sizeof(tmp)-1;
      memcpy(tmp, display_name, copy_n);
      file.write(tmp, sizeof(tmp));
      file.close();
      return true;
    }
    file.close();
  }
  return false;
#else
  File file = _fs->open(filename, "w", true);
#endif
  if (file) {
#if !defined(PLATFORM_NATIVE)
    mesh::LocalIdentity temp_id = id;  // Create a non-const copy
    bool success = temp_id.writeTo(file);
    if (success) {
      uint8_t tmp[32];
      memset(tmp, 0, sizeof(tmp));
      int n = strlen(display_name);
      if (n > sizeof(tmp)-1) n = sizeof(tmp)-1;
      memcpy(tmp, display_name, n);
      file.write(tmp, sizeof(tmp));
      file.close();
      return true;
    }
#endif
    file.close();
  }
  return false;
}
