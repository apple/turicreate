/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFileMonitor.h"

#include "cmAlgorithms.h"
#include "cmsys/SystemTools.hxx"

#include <cassert>
#include <stddef.h>
#include <unordered_map>
#include <utility>

namespace {
void on_directory_change(uv_fs_event_t* handle, const char* filename,
                         int events, int status);
void on_fs_close(uv_handle_t* handle);
} // namespace

class cmIBaseWatcher
{
public:
  cmIBaseWatcher() = default;
  virtual ~cmIBaseWatcher() = default;

  virtual void Trigger(const std::string& pathSegment, int events,
                       int status) const = 0;
  virtual std::string Path() const = 0;
  virtual uv_loop_t* Loop() const = 0;

  virtual void StartWatching() = 0;
  virtual void StopWatching() = 0;

  virtual std::vector<std::string> WatchedFiles() const = 0;
  virtual std::vector<std::string> WatchedDirectories() const = 0;
};

class cmVirtualDirectoryWatcher : public cmIBaseWatcher
{
public:
  ~cmVirtualDirectoryWatcher() override { cmDeleteAll(this->Children); }

  cmIBaseWatcher* Find(const std::string& ps)
  {
    const auto i = this->Children.find(ps);
    return (i == this->Children.end()) ? nullptr : i->second;
  }

  void Trigger(const std::string& pathSegment, int events,
               int status) const final
  {
    if (pathSegment.empty()) {
      for (const auto& i : this->Children) {
        i.second->Trigger(std::string(), events, status);
      }
    } else {
      const auto i = this->Children.find(pathSegment);
      if (i != this->Children.end()) {
        i->second->Trigger(std::string(), events, status);
      }
    }
  }

  void StartWatching() override
  {
    for (const auto& i : this->Children) {
      i.second->StartWatching();
    }
  }

  void StopWatching() override
  {
    for (const auto& i : this->Children) {
      i.second->StopWatching();
    }
  }

  std::vector<std::string> WatchedFiles() const final
  {
    std::vector<std::string> result;
    for (const auto& i : this->Children) {
      for (const auto& j : i.second->WatchedFiles()) {
        result.push_back(j);
      }
    }
    return result;
  }

  std::vector<std::string> WatchedDirectories() const override
  {
    std::vector<std::string> result;
    for (const auto& i : this->Children) {
      for (const auto& j : i.second->WatchedDirectories()) {
        result.push_back(j);
      }
    }
    return result;
  }

  void Reset()
  {
    cmDeleteAll(this->Children);
    this->Children.clear();
  }

  void AddChildWatcher(const std::string& ps, cmIBaseWatcher* watcher)
  {
    assert(!ps.empty());
    assert(this->Children.find(ps) == this->Children.end());
    assert(watcher);

    this->Children.emplace(std::make_pair(ps, watcher));
  }

private:
  std::unordered_map<std::string, cmIBaseWatcher*> Children; // owned!
};

// Root of all the different (on windows!) root directories:
class cmRootWatcher : public cmVirtualDirectoryWatcher
{
public:
  cmRootWatcher(uv_loop_t* loop)
    : mLoop(loop)
  {
    assert(loop);
  }

  std::string Path() const final
  {
    assert(false);
    return std::string();
  }
  uv_loop_t* Loop() const final { return this->mLoop; }

private:
  uv_loop_t* const mLoop; // no ownership!
};

// Real directories:
class cmRealDirectoryWatcher : public cmVirtualDirectoryWatcher
{
public:
  cmRealDirectoryWatcher(cmVirtualDirectoryWatcher* p, const std::string& ps)
    : Parent(p)
    , PathSegment(ps)
  {
    assert(p);
    assert(!ps.empty());

    p->AddChildWatcher(ps, this);
  }

  ~cmRealDirectoryWatcher() override
  {
    // Handle is freed via uv_handle_close callback!
  }

  void StartWatching() final
  {
    if (!this->Handle) {
      this->Handle = new uv_fs_event_t;

      uv_fs_event_init(this->Loop(), this->Handle);
      this->Handle->data = this;
      uv_fs_event_start(this->Handle, &on_directory_change, Path().c_str(), 0);
    }
    cmVirtualDirectoryWatcher::StartWatching();
  }

  void StopWatching() final
  {
    if (this->Handle) {
      uv_fs_event_stop(this->Handle);
      uv_close(reinterpret_cast<uv_handle_t*>(this->Handle), &on_fs_close);
      this->Handle = nullptr;
    }
    cmVirtualDirectoryWatcher::StopWatching();
  }

  uv_loop_t* Loop() const final { return this->Parent->Loop(); }

  std::vector<std::string> WatchedDirectories() const override
  {
    std::vector<std::string> result = { Path() };
    for (const auto& j : cmVirtualDirectoryWatcher::WatchedDirectories()) {
      result.push_back(j);
    }
    return result;
  }

protected:
  cmVirtualDirectoryWatcher* const Parent;
  const std::string PathSegment;

private:
  uv_fs_event_t* Handle = nullptr; // owner!
};

// Root directories:
class cmRootDirectoryWatcher : public cmRealDirectoryWatcher
{
public:
  cmRootDirectoryWatcher(cmRootWatcher* p, const std::string& ps)
    : cmRealDirectoryWatcher(p, ps)
  {
  }

  std::string Path() const final { return this->PathSegment; }
};

// Normal directories below root:
class cmDirectoryWatcher : public cmRealDirectoryWatcher
{
public:
  cmDirectoryWatcher(cmRealDirectoryWatcher* p, const std::string& ps)
    : cmRealDirectoryWatcher(p, ps)
  {
  }

  std::string Path() const final
  {
    return this->Parent->Path() + this->PathSegment + "/";
  }
};

class cmFileWatcher : public cmIBaseWatcher
{
public:
  cmFileWatcher(cmRealDirectoryWatcher* p, const std::string& ps,
                cmFileMonitor::Callback cb)
    : Parent(p)
    , PathSegment(ps)
    , CbList({ std::move(cb) })
  {
    assert(p);
    assert(!ps.empty());
    p->AddChildWatcher(ps, this);
  }

  void StartWatching() final {}

  void StopWatching() final {}

  void AppendCallback(cmFileMonitor::Callback const& cb)
  {
    this->CbList.push_back(cb);
  }

  std::string Path() const final
  {
    return this->Parent->Path() + this->PathSegment;
  }

  std::vector<std::string> WatchedDirectories() const final { return {}; }

  std::vector<std::string> WatchedFiles() const final
  {
    return { this->Path() };
  }

  void Trigger(const std::string& ps, int events, int status) const final
  {
    assert(ps.empty());
    assert(status == 0);
    static_cast<void>(ps);

    const std::string path = this->Path();
    for (const auto& cb : this->CbList) {
      cb(path, events, status);
    }
  }

  uv_loop_t* Loop() const final { return this->Parent->Loop(); }

private:
  cmRealDirectoryWatcher* Parent;
  const std::string PathSegment;
  std::vector<cmFileMonitor::Callback> CbList;
};

namespace {

void on_directory_change(uv_fs_event_t* handle, const char* filename,
                         int events, int status)
{
  const cmIBaseWatcher* const watcher =
    static_cast<const cmIBaseWatcher*>(handle->data);
  const std::string pathSegment(filename ? filename : "");
  watcher->Trigger(pathSegment, events, status);
}

void on_fs_close(uv_handle_t* handle)
{
  delete reinterpret_cast<uv_fs_event_t*>(handle);
}

} // namespace

cmFileMonitor::cmFileMonitor(uv_loop_t* l)
  : Root(new cmRootWatcher(l))
{
}

cmFileMonitor::~cmFileMonitor()
{
  delete this->Root;
}

void cmFileMonitor::MonitorPaths(const std::vector<std::string>& paths,
                                 Callback const& cb)
{
  for (const auto& p : paths) {
    std::vector<std::string> pathSegments;
    cmsys::SystemTools::SplitPath(p, pathSegments, true);

    const size_t segmentCount = pathSegments.size();
    if (segmentCount < 2) { // Expect at least rootdir and filename
      continue;
    }
    cmVirtualDirectoryWatcher* currentWatcher = this->Root;
    for (size_t i = 0; i < segmentCount; ++i) {
      assert(currentWatcher);

      const bool fileSegment = (i == segmentCount - 1);
      const bool rootSegment = (i == 0);
      assert(
        !(fileSegment &&
          rootSegment)); // Can not be both filename and root part of the path!

      const std::string& currentSegment = pathSegments[i];
      if (currentSegment.empty()) {
        continue;
      }

      cmIBaseWatcher* nextWatcher = currentWatcher->Find(currentSegment);
      if (!nextWatcher) {
        if (rootSegment) { // Root part
          assert(currentWatcher == this->Root);
          nextWatcher = new cmRootDirectoryWatcher(this->Root, currentSegment);
          assert(currentWatcher->Find(currentSegment) == nextWatcher);
        } else if (fileSegment) { // File part
          assert(currentWatcher != this->Root);
          nextWatcher = new cmFileWatcher(
            dynamic_cast<cmRealDirectoryWatcher*>(currentWatcher),
            currentSegment, cb);
          assert(currentWatcher->Find(currentSegment) == nextWatcher);
        } else { // Any normal directory in between
          nextWatcher = new cmDirectoryWatcher(
            dynamic_cast<cmRealDirectoryWatcher*>(currentWatcher),
            currentSegment);
          assert(currentWatcher->Find(currentSegment) == nextWatcher);
        }
      } else {
        if (fileSegment) {
          auto filePtr = dynamic_cast<cmFileWatcher*>(nextWatcher);
          assert(filePtr);
          filePtr->AppendCallback(cb);
          continue;
        }
      }
      currentWatcher = dynamic_cast<cmVirtualDirectoryWatcher*>(nextWatcher);
    }
  }
  this->Root->StartWatching();
}

void cmFileMonitor::StopMonitoring()
{
  this->Root->StopWatching();
  this->Root->Reset();
}

std::vector<std::string> cmFileMonitor::WatchedFiles() const
{
  std::vector<std::string> result;
  if (this->Root) {
    result = this->Root->WatchedFiles();
  }
  return result;
}

std::vector<std::string> cmFileMonitor::WatchedDirectories() const
{
  std::vector<std::string> result;
  if (this->Root) {
    result = this->Root->WatchedDirectories();
  }
  return result;
}
