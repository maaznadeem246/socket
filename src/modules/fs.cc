module;
#include <socket/platform.hh>
#include <map>

/**
 * @TODO
 */
export module ssc.fs;
import ssc.data;
import ssc.headers;
import ssc.json;
import ssc.log;
import ssc.loop;
import ssc.peer;
import ssc.timers;
import ssc.types;
import ssc.utils;
import ssc.uv;

using namespace ssc::data;
using namespace ssc::headers;
using namespace ssc::loop;
using namespace ssc::peer;
using namespace ssc::types;
using namespace ssc::timers;
using namespace ssc::utils;

export namespace ssc::fs {
  // forward
  class FS;
  class Descriptor;
  class DescriptorManager;

  struct Descriptor {
    uint64_t id;
    Mutex mutex;
    AtomicBool retained = false;
    AtomicBool stale = false;
    uv_dir_t *dir = nullptr;
    uv_file fd = 0;
    FS* fs = nullptr;

    DescriptorManager* descriptorManager = nullptr;

    Descriptor (uint64_t id, DescriptorManager* descriptorManager) {
      this->id = id;
      this->descriptorManager = descriptorManager;
    }

    bool isDirectory () {
      return this->dir != nullptr;
    }

    bool isFile () {
      Lock lock(this->mutex);
      return this->fd > 0 && this->dir == nullptr;
    }

    bool isRetained () {
      Lock lock(this->mutex);
      return this->retained;
    }

    bool isStale () {
      Lock lock(this->mutex);
      return this->stale;
    }
  };

  struct RequestContext {
    using Callback = std::function<void(String, JSON::Any, Data)>;

    uint64_t id;
    String seq;
    Descriptor *desc = nullptr;
    Callback callback = nullptr;
    uv_fs_t req;
    uv_buf_t iov[16];
    // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
    uv_dirent_t dirents[256];
    int offset = 0;
    int result = 0;
    FS* fs = nullptr;

    RequestContext () = default;
    RequestContext (Descriptor *desc)
      : RequestContext(desc, "", nullptr) {}
    RequestContext (String seq, Callback callback)
      : RequestContext(nullptr, seq, callback) {}
    RequestContext (Descriptor *desc, String seq, Callback callback) {
      this->id = rand64();
      this->callback = callback;
      this->seq = seq;
      this->desc = desc;
      this->req.data = (void *) this;
      if (desc != nullptr) {
        this->fs = desc->fs;
      }
    }

    ~RequestContext () {
      uv_fs_req_cleanup(&this->req);
    }

    void setBuffer (int index, size_t len, char *base) {
      this->iov[index].base = base;
      this->iov[index].len = len;
    }

    void freeBuffer (int index) {
      if (this->iov[index].base != nullptr) {
        delete [] (char *) this->iov[index].base;
        this->iov[index].base = nullptr;
      }

      this->iov[index].len = 0;
    }

    char* getBuffer (int index) {
      return this->iov[index].base;
    }

    size_t getBufferSize (int index) {
      return this->iov[index].len;
    }
  };

  class DescriptorManager {
    public:
      using Callback = RequestContext::Callback;
      using Descriptors = std::map<uint64_t, Descriptor*>;

      Descriptors descriptors;
      Timers& timers;
      Mutex mutex;
      Loop& loop;
      FS* fs = nullptr;

      DescriptorManager () = delete;
      DescriptorManager (Loop& loop, Timers& timers, FS* fs)
        : timers(timers),
          loop(loop)
      {
        this->fs = fs;
      }

      void init ();

      Descriptor* create (uint64_t id) {
        Lock lock(this->mutex);

        if (this->has(id)) {
          return this->get(id);
        }

        auto descriptor = new Descriptor(id, this);
        descriptors.insert_or_assign(id, descriptor);

        return descriptor;
      }

      Descriptor* get (uint64_t id) {
        Lock lock(this->mutex);

        if (descriptors.find(id) != descriptors.end()) {
          return descriptors.at(id);
        }
        return nullptr;
      }

      void remove (uint64_t id) {
        Lock lock(this->mutex);
        if (descriptors.find(id) != descriptors.end()) {
          auto descriptor = this->get(id);
          descriptors.erase(id);
          if (descriptor != nullptr) {
            delete descriptor;
          }
        }
      }

      bool has (uint64_t id) {
        Lock lock(this->mutex);
        return descriptors.find(id) != descriptors.end();
      }

      auto count () {
        Lock lock(this->mutex);
        return this->descriptors.size();
      }
  };

  class FS {
    public:
      using Callback = RequestContext::Callback;

      DescriptorManager& descriptorManager;
      Timers& timers;
      Loop& loop;

      FS (Loop& loop, DescriptorManager& descriptorManager, Timers& timers)
        : descriptorManager(descriptorManager),
          timers(timers),
          loop(loop)
      {}

      void constants (const String seq, Callback callback);
      void access (
        const String seq,
        const String path,
        int mode,
        Callback callback
      );
      void chmod (
        const String seq,
        const String path,
        int mode,
        Callback callback
      );
      void close (const String seq, uint64_t id, Callback callback);
      void copyFile (
        const String seq,
        const String src,
        const String dst,
        int mode,
        Callback callback
      );
      void closedir (const String seq, uint64_t id, Callback callback);
      void closeOpenDescriptor (
        const String seq,
        uint64_t id,
        Callback callback
      );
      void closeOpenDescriptors (const String seq, Callback callback);
      void closeOpenDescriptors (
        const String seq,
        bool preserveRetained,
        Callback callback
      );
      void fstat (const String seq, uint64_t id, Callback callback);
      void getOpenDescriptors (const String seq, Callback callback);
      void lstat (const String seq, const String path, Callback callback);
      void mkdir (
        const String seq,
        const String path,
        int mode,
        Callback callback
      );
      void open (
        const String seq,
        uint64_t id,
        const String path,
        int flags,
        int mode,
        Callback callback
      );
      void opendir (
        const String seq,
        uint64_t id,
        const String path,
        Callback callback
      );
      void read (
        const String seq,
        uint64_t id,
        size_t len,
        size_t offset,
        Callback callback
      );
      void readdir (
        const String seq,
        uint64_t id,
        size_t entries,
        Callback callback
      );
      void retainOpenDescriptor (
        const String seq,
        uint64_t id,
        Callback callback
      );
      void rename (
        const String seq,
        const String src,
        const String dst,
        Callback callback
      );
      void rmdir (
        const String seq,
        const String path,
        Callback callback
      );
      void stat (
        const String seq,
        const String path,
        Callback callback
      );
      void unlink (
        const String seq,
        const String path,
        Callback callback
      );
      void write (
        const String seq,
        uint64_t id,
        char *bytes,
        size_t size,
        size_t offset,
        Callback callback
      );
  };

  inline void DescriptorManager::init () {
    static Timer releaseWeakDescriptors = {
      .timeout = 1024, // in milliseconds
      .invoke = [](uv_timer_t *handle) {
        auto descriptorManager = reinterpret_cast<DescriptorManager*>(handle->data);
        Vector<uint64_t> ids;

        log::info("in timer");
        {
          //Lock lock(descriptorManager->mutex);
          for (const auto& tuple : descriptorManager->descriptors) {
            ids.push_back(tuple.first);
          }
        }

        for (auto const id : ids) {
          auto desc = descriptorManager->get(id);

          if (desc == nullptr) {
            descriptorManager->remove(id);
            continue;
          }

          if (desc->isRetained() || !desc->isStale()) {
            continue;
          }

          if (desc->isDirectory()) {
            descriptorManager->fs->closedir("", id, [](auto seq, auto msg, auto data) {});
          } else if (desc->isFile()) {
            descriptorManager->fs->close("", id, [](auto seq, auto msg, auto data) {});
          } else {
            // free
            descriptorManager->remove(id);
          }
        }

        log::info("finish timer");
      }
    };

    //this->timers.add(releaseWeakDescriptors, this);
  }

  #define SET_CONSTANT(c) constants[#c] = (c);
  std::map<String, int32_t> getFSConstantsMap () {
    std::map<String, int32_t> constants;

    #if defined(UV_DIRENT_UNKNOWN)
      SET_CONSTANT(UV_DIRENT_UNKNOWN)
    #endif
    #if defined(UV_DIRENT_FILE)
      SET_CONSTANT(UV_DIRENT_FILE)
    #endif
    #if defined(UV_DIRENT_DIR)
      SET_CONSTANT(UV_DIRENT_DIR)
    #endif
    #if defined(UV_DIRENT_LINK)
      SET_CONSTANT(UV_DIRENT_LINK)
    #endif
    #if defined(UV_DIRENT_FIFO)
      SET_CONSTANT(UV_DIRENT_FIFO)
    #endif
    #if defined(UV_DIRENT_SOCKET)
      SET_CONSTANT(UV_DIRENT_SOCKET)
    #endif
    #if defined(UV_DIRENT_CHAR)
      SET_CONSTANT(UV_DIRENT_CHAR)
    #endif
    #if defined(UV_DIRENT_BLOCK)
      SET_CONSTANT(UV_DIRENT_BLOCK)
    #endif
    #if defined(O_RDONLY)
      SET_CONSTANT(O_RDONLY);
    #endif
    #if defined(O_WRONLY)
      SET_CONSTANT(O_WRONLY);
    #endif
    #if defined(O_RDWR)
      SET_CONSTANT(O_RDWR);
    #endif
    #if defined(O_APPEND)
      SET_CONSTANT(O_APPEND);
    #endif
    #if defined(O_ASYNC)
      SET_CONSTANT(O_ASYNC);
    #endif
    #if defined(O_CLOEXEC)
      SET_CONSTANT(O_CLOEXEC);
    #endif
    #if defined(O_CREAT)
      SET_CONSTANT(O_CREAT);
    #endif
    #if defined(O_DIRECT)
      SET_CONSTANT(O_DIRECT);
    #endif
    #if defined(O_DIRECTORY)
      SET_CONSTANT(O_DIRECTORY);
    #endif
    #if defined(O_DSYNC)
      SET_CONSTANT(O_DSYNC);
    #endif
    #if defined(O_EXCL)
      SET_CONSTANT(O_EXCL);
    #endif
    #if defined(O_LARGEFILE)
      SET_CONSTANT(O_LARGEFILE);
    #endif
    #if defined(O_NOATIME)
      SET_CONSTANT(O_NOATIME);
    #endif
    #if defined(O_NOCTTY)
      SET_CONSTANT(O_NOCTTY);
    #endif
    #if defined(O_NOFOLLOW)
      SET_CONSTANT(O_NOFOLLOW);
    #endif
    #if defined(O_NONBLOCK)
      SET_CONSTANT(O_NONBLOCK);
    #endif
    #if defined(O_NDELAY)
      SET_CONSTANT(O_NDELAY);
    #endif
    #if defined(O_PATH)
      SET_CONSTANT(O_PATH);
    #endif
    #if defined(O_SYNC)
      SET_CONSTANT(O_SYNC);
    #endif
    #if defined(O_TMPFILE)
      SET_CONSTANT(O_TMPFILE);
    #endif
    #if defined(O_TRUNC)
      SET_CONSTANT(O_TRUNC);
    #endif
    #if defined(S_IFMT)
      SET_CONSTANT(S_IFMT);
    #endif
    #if defined(S_IFREG)
      SET_CONSTANT(S_IFREG);
    #endif
    #if defined(S_IFDIR)
      SET_CONSTANT(S_IFDIR);
    #endif
    #if defined(S_IFCHR)
      SET_CONSTANT(S_IFCHR);
    #endif
    #if defined(S_IFBLK)
      SET_CONSTANT(S_IFBLK);
    #endif
    #if defined(S_IFIFO)
      SET_CONSTANT(S_IFIFO);
    #endif
    #if defined(S_IFLNK)
      SET_CONSTANT(S_IFLNK);
    #endif
    #if defined(S_IFSOCK)
      SET_CONSTANT(S_IFSOCK);
    #endif
    #if defined(S_IRWXU)
      SET_CONSTANT(S_IRWXU);
    #endif
    #if defined(S_IRUSR)
      SET_CONSTANT(S_IRUSR);
    #endif
    #if defined(S_IWUSR)
      SET_CONSTANT(S_IWUSR);
    #endif
    #if defined(S_IXUSR)
      SET_CONSTANT(S_IXUSR);
    #endif
    #if defined(S_IRWXG)
      SET_CONSTANT(S_IRWXG);
    #endif
    #if defined(S_IRGRP)
      SET_CONSTANT(S_IRGRP);
    #endif
    #if defined(S_IWGRP)
      SET_CONSTANT(S_IWGRP);
    #endif
    #if defined(S_IXGRP)
      SET_CONSTANT(S_IXGRP);
    #endif
    #if defined(S_IRWXO)
      SET_CONSTANT(S_IRWXO);
    #endif
    #if defined(S_IROTH)
      SET_CONSTANT(S_IROTH);
    #endif
    #if defined(S_IWOTH)
      SET_CONSTANT(S_IWOTH);
    #endif
    #if defined(S_IXOTH)
      SET_CONSTANT(S_IXOTH);
    #endif
    #if defined(F_OK)
      SET_CONSTANT(F_OK);
    #endif
    #if defined(R_OK)
      SET_CONSTANT(R_OK);
    #endif
    #if defined(W_OK)
      SET_CONSTANT(W_OK);
    #endif
    #if defined(X_OK)
      SET_CONSTANT(X_OK);
    #endif

    return constants;
  }
  #undef SET_CONSTANT

  JSON::Object getStatsJSON (const String& source, uv_stat_t* stats) {
    return JSON::Object::Entries {
      {"source", source},
      {"data", JSON::Object::Entries {
        {"st_dev", std::to_string(stats->st_dev)},
        {"st_mode", std::to_string(stats->st_mode)},
        {"st_nlink", std::to_string(stats->st_nlink)},
        {"st_uid", std::to_string(stats->st_uid)},
        {"st_gid", std::to_string(stats->st_gid)},
        {"st_rdev", std::to_string(stats->st_rdev)},
        {"st_ino", std::to_string(stats->st_ino)},
        {"st_size", std::to_string(stats->st_size)},
        {"st_blksize", std::to_string(stats->st_blksize)},
        {"st_blocks", std::to_string(stats->st_blocks)},
        {"st_flags", std::to_string(stats->st_flags)},
        {"st_gen", std::to_string(stats->st_gen)},
        {"st_atim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_atim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_atim.tv_nsec)},
        }},
        {"st_mtim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_mtim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_mtim.tv_nsec)}
        }},
        {"st_ctim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_ctim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_ctim.tv_nsec)}
        }},
        {"st_birthtim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_birthtim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_birthtim.tv_nsec)}
        }}
      }}
    };
  }

  void FS::access (
    const String seq,
    const String path,
    int mode,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_access(loop.get(), req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.access"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.access"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.access"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::chmod (
    const String seq,
    const String path,
    int mode,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_chmod(loop.get(), req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.chmod"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.chmod"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.chmod"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::close (
    const String seq,
    uint64_t id,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto desc = this->descriptorManager.get(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_close(loop.get(), req, desc->fd, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.close"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.close"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"fd", desc->fd}
            }}
          };

          desc->descriptorManager->remove(desc->id);
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::open (
    const String seq,
    uint64_t id,
    const String path,
    int flags,
    int mode,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto desc = this->descriptorManager.create(id);
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_open(loop.get(), req, filename, flags, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.open"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };

          desc->descriptorManager->remove(desc->id);
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.open"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"fd", (int) req->result}
            }}
          };

          desc->fd = (int) req->result;
          // insert into `descriptors` map
          Lock lock(desc->descriptorManager->mutex);
          desc->descriptorManager->descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.open"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        desc->descriptorManager->remove(desc->id);
        delete ctx;
      }
    });
  }

  void FS::opendir (
    const String seq,
    uint64_t id,
    const String path,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto desc =  new Descriptor(id, &this->descriptorManager);
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_opendir(loop.get(), req, filename, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.opendir"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };

          delete desc;
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.opendir"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)}
            }}
          };

          desc->dir = (uv_dir_t *) req->ptr;
          // insert into `descriptors` map
          Lock lock(desc->descriptorManager->mutex);
          desc->descriptorManager->descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.opendir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete desc;
        delete ctx;
      }
    });
  }

  void FS::readdir (
    const String seq,
    uint64_t id,
    size_t nentries,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto desc = this->descriptorManager.get(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      if (!desc->isDirectory()) {
        auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      Lock lock(desc->mutex);
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;

      desc->dir->dirents = ctx->dirents;
      desc->dir->nentries = nentries;

      auto err = uv_fs_readdir(loop.get(), req, desc->dir, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.readdir"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          Vector<JSON::Any> entries;

          for (int i = 0; i < req->result; ++i) {
            auto entry = JSON::Object::Entries {
              {"type", desc->dir->dirents[i].type},
              {"name", desc->dir->dirents[i].name}
            };

            entries.push_back(entry);
          }

          json = JSON::Object::Entries {
            {"source", "fs.readdir"},
            {"data", entries}
          };
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.readdir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::closedir (
    const String seq,
    uint64_t id,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto desc = this->descriptorManager.get(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      if (!desc->isDirectory()) {
        auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_closedir(loop.get(), req, desc->dir, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.closedir"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.closedir"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"fd", desc->fd}
            }}
          };

          desc->descriptorManager->remove(desc->id);
          delete desc;
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::closeOpenDescriptor (
    const String seq,
    uint64_t id,
    Callback callback
  ) {
    auto desc = this->descriptorManager.get(id);

    if (desc == nullptr) {
      auto json = JSON::Object::Entries {
        {"source", "fs.closeOpenDescriptor"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"code", "ENOTOPEN"},
          {"type", "NotFoundError"},
          {"message", "No descriptor found with that id"}
        }}
      };

      return callback(seq, json, Data{});
    }

    if (desc->isDirectory()) {
      this->closedir(seq, id, callback);
    } else if (desc->isFile()) {
      this->close(seq, id, callback);
    }
  }

  void FS::closeOpenDescriptors (const String seq, Callback callback) {
    return this->closeOpenDescriptors(seq, false, callback);
  }

  void FS::closeOpenDescriptors (
    const String seq,
    bool preserveRetained,
    Callback callback
  ) {
    auto pending = this->descriptorManager.count();
    int queued = 0;
    auto json = JSON::Object {};
    auto ids = Vector<uint64_t> {};

    {
      Lock lock(this->descriptorManager.mutex);
      for (auto const &tuple : this->descriptorManager.descriptors) {
        ids.push_back(tuple.first);
      }
    }

    for (auto const id : ids) {
      auto desc = this->descriptorManager.get(id);
      pending--;

      if (desc == nullptr) {
        this->descriptorManager.remove(id);
        continue;
      }

      if (preserveRetained && desc->isRetained()) {
        continue;
      }

      if (desc->isDirectory()) {
        queued++;
        this->closedir(seq, id, [pending, callback](auto seq, auto json, auto data) {
          if (pending == 0) {
            callback(seq, json, data);
          }
        });
      } else if (desc->isFile()) {
        queued++;
        this->close(seq, id, [pending, callback](auto seq, auto json, auto data) {
          if (pending == 0) {
            callback(seq, json, data);
          }
        });
      }
    }

    if (queued == 0) {
      callback(seq, json, Data{});
    }
  }

  void FS::read (
    const String seq,
    uint64_t id,
    size_t size,
    size_t offset,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto desc = this->descriptorManager.get(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.read"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto bytes = new char[size]{0};

      ctx->setBuffer(0, size, bytes);

      auto err = uv_fs_read(loop.get(), req, desc->fd, ctx->iov, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto desc = ctx->desc;
        auto json = JSON::Object {};
        Data data = {0};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.read"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };

          auto bytes = ctx->getBuffer(0);
          if (bytes != nullptr) {
            delete [] bytes;
          }
        } else {
          auto headers = Headers {{
            {"content-type" ,"application/octet-stream"},
            {"content-length", req->result}
          }};

          data.id = rand64();
          data.body = ctx->getBuffer(0);
          data.length = (int) req->result;
          data.headers = headers.str();
          data.bodyNeedsFree = true;
        }

        ctx->callback(ctx->seq, json, data);
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.read"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete [] bytes;
        delete ctx;
      }
    });
  }

  void FS::write (
    const String seq,
    uint64_t id,
    char *bytes,
    size_t size,
    size_t offset,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto desc = this->descriptorManager.get(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.write"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;

      ctx->setBuffer(0, (int) size, bytes);
      auto err = uv_fs_write(loop.get(), req, desc->fd, ctx->iov, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.write"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.write"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"result", req->result}
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.write"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::stat (
    const String seq,
    const String path,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_stat(loop.get(), req, filename, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.stat"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = getStatsJSON("fs.stat", uv_fs_get_statbuf(req));
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::fstat (
    const String seq,
    uint64_t id,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto desc = this->descriptorManager.get(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.fstat"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Data{});
      }

      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_fstat(loop.get(), req, desc->fd, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.fstat"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = getStatsJSON("fs.fstat", uv_fs_get_statbuf(req));
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.fstat"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::retainOpenDescriptor (
    const String seq,
    uint64_t id,
    Callback callback
  ) {
    auto desc = this->descriptorManager.get(id);

    if (desc == nullptr) {
      auto json = JSON::Object::Entries {
        {"source", "fs.retainOpenDescriptor"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"code", "ENOTOPEN"},
          {"type", "NotFoundError"},
          {"message", "No file descriptor found with that id"}
        }}
      };

      return callback(seq, json, Data{});
    }

    Lock lock(desc->mutex);
    desc->retained = true;
    auto json = JSON::Object::Entries {
      {"source", "fs.retainOpenDescriptor"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(desc->id)}
      }}
    };

    callback(seq, json, Data{});
  }

  void FS::getOpenDescriptors (
    const String seq,
    Callback callback
  ) {
    auto entries = Vector<JSON::Any> {};

    Lock lock(this->descriptorManager.mutex);
    for (auto const &tuple : this->descriptorManager.descriptors) {
      auto desc = tuple.second;

      if (!desc || (desc->isStale() && !desc->isRetained())) {
        continue;
      }

      auto entry = JSON::Object::Entries {
        {"id",  std::to_string(desc->id)},
        {"fd", std::to_string(desc->isDirectory() ? desc->id : desc->fd)},
        {"type", desc->dir ? "directory" : "file"}
      };

      entries.push_back(entry);
    }

    auto json = JSON::Object::Entries {
      {"source", "fs.getOpenDescriptors"},
      {"data", entries}
    };

    callback(seq, json, Data{});
  }

  void FS::lstat (
    const String seq,
    const String path,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_lstat(loop.get(), req, filename, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.stat"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = getStatsJSON("fs.lstat", uv_fs_get_statbuf(req));
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::unlink (
    const String seq,
    const String path,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_unlink(loop.get(), req, filename, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.unlink"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.unlink"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.unlink"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::rename (
    const String seq,
    const String pathA,
    const String pathB,
    const Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto src = pathA.c_str();
      auto dst = pathB.c_str();
      auto err = uv_fs_rename(loop.get(), req, src, dst, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.rename"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.rename"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.rename"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::copyFile (
    const String seq,
    const String pathA,
    const String pathB,
    int flags,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto src = pathA.c_str();
      auto dst = pathB.c_str();
      auto err = uv_fs_copyfile(loop.get(), req, src, dst, flags, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.copyFile"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.copyFile"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.copyFile"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::rmdir (
    const String seq,
    const String path,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_rmdir(loop.get(), req, filename, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.rmdir"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.rmdir"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.rmdir"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::mkdir (
    const String seq,
    const String path,
    int mode,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto filename = path.c_str();
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_mkdir(loop.get(), req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (req->result < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.mkdir"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.mkdir"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.mkdir"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Data{});
        delete ctx;
      }
    });
  }

  void FS::constants (const String seq, Callback callback) {
    static auto constants = getFSConstantsMap();

    this->loop.dispatch([=] {
      JSON::Object::Entries data;

      for (auto const &tuple : constants) {
        auto key = tuple.first;
        auto value = tuple.second;
        data[key] = value;
      }

      auto json = JSON::Object::Entries {
        {"source", "fs.constants"},
        {"data", data}
      };

      callback(seq, json, Data{});
    });
  }
}
