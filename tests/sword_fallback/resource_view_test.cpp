// Production Ship manager/cache/loader bodies; archive mounting/I/O and
// unused XML parsing are seams. No game runtime is claimed.
#include <cassert>
#include <atomic>
#include <chrono>
#include <cstring>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <shared_mutex>
#include "ship/resource/CrossRMRegistry.h"
#include "combo/NeiAssetPriority.h"
#define COMBO_EXPORT
#include <nlohmann/json.hpp>
// The runner wraps only the private mutex field to observe member lifetime.
// Its lock behavior stays native; ASan alone cannot detect a destroyed mutex
// while the enclosing ResourceManager allocation remains alive.
static std::atomic<bool> resourceLoadBlocked = false;
struct ResourceLifetimeMutex : std::mutex {
    ~ResourceLifetimeMutex() {
        assert(!resourceLoadBlocked.load() && "manager state destroyed before its pending load finished");
    }
};
#include "ship/resource/ResourceManager.h"
#include "ship/utils/Utils.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include "fast/RenderResourceLookup.h"
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_TRACE(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
namespace Ship {
class Context {
  public:
    static Context* GetRawInstance(){static Context context;return &context;}
    std::shared_ptr<ResourceManager> GetResourceManager(){return active;}
    void SetResourceManager(std::shared_ptr<ResourceManager> value){active=std::move(value);}
    static std::string LocateFileAcrossAppDirs(const char*,const char* = nullptr){return shipped;}
    inline static std::string shipped="/game/soh.o2r";
    inline static std::shared_ptr<ResourceManager> active;
};
namespace PerformanceTrace {
struct Context{bool enabled=false;};
struct Scope{template<class... T> Scope(T&&...){} void Detail(const char*){}};
struct ContextScope{explicit ContextScope(Context){}};
inline Context CaptureContext(){return {};}
inline uint64_t Now(){return 0;}
template<class... T> void Record(T&&...){}
inline void Count(const char*){}
}
class MemoryArchive:public Archive {
  public:
    explicit MemoryArchive(const char* path):Archive(path){}
    std::map<std::string,std::vector<char>> files;
    void Put(const std::string& path,std::vector<char> bytes){files[path]=std::move(bytes);IndexFile(path);}
    std::shared_ptr<File> LoadFile(const std::string& path) override {
        auto it=files.find(path);if(it==files.end())return nullptr;
        auto file=std::make_shared<File>();file->Buffer=std::make_shared<std::vector<char>>(it->second);
        file->IsLoaded=true;return file;
    }
    std::shared_ptr<File> LoadFile(uint64_t hash) override {
        auto names=ListFiles();auto it=names->find(hash);return it==names->end()?nullptr:LoadFile(it->second);
    }
    bool Open() override{return true;}
    bool Close() override{return true;}
    bool WriteFile(const std::string&,const std::vector<uint8_t>&) override{return false;}
};
static std::vector<std::shared_ptr<Archive>> mounted;
Archive::Archive(const std::string& path):mPath(path),mHashes(std::make_shared<std::unordered_map<uint64_t,std::string>>()){}
Archive::~Archive()=default;
void Archive::IndexFile(const std::string& path){(*mHashes)[std::hash<std::string>{}(path)]=path;}
bool Archive::HasFile(const std::string& path){return HasFile(std::hash<std::string>{}(path));}
bool Archive::HasFile(uint64_t hash){return mHashes->contains(hash);}
const std::string& Archive::GetPath(){return mPath;}
std::shared_ptr<std::unordered_map<uint64_t,std::string>> Archive::ListFiles(){return mHashes;}
ArchiveManager::ArchiveManager()=default;
ArchiveManager::~ArchiveManager()=default;
void ArchiveManager::Init(const std::vector<std::string>&,const std::unordered_set<uint32_t>&){
    mArchives=mounted;
    for(const auto& archive:mArchives)for(const auto& [hash,path]:*archive->ListFiles())mHashes[hash]=path;
}
bool ArchiveManager::IsLoaded(){return !mArchives.empty();}
std::shared_ptr<std::vector<std::shared_ptr<Archive>>> ArchiveManager::GetArchives(){
    return std::make_shared<std::vector<std::shared_ptr<Archive>>>(mArchives);
}
std::shared_ptr<File> ArchiveManager::LoadFile(const std::string& path){
    for(auto it=mArchives.rbegin();it!=mArchives.rend();++it)if(auto file=(*it)->LoadFile(path))return file;
    return nullptr;
}
bool ArchiveManager::HasFile(const std::string& path){
    for(const auto& archive:mArchives)if(archive->HasFile(path))return true;return false;
}
int32_t ArchiveManager::GetFilePriority(const std::string& path){
    for(int32_t i=int32_t(mArchives.size())-1;i>=0;--i)if(mArchives[i]->HasFile(path))return i;return -1;
}
const std::string* ArchiveManager::HashToString(uint64_t hash) const {
    auto it=mHashes.find(hash);return it==mHashes.end()?nullptr:&it->second;
}
// Only unused global/XML importers are replaced. Binary header/metadata,
// aliases, registered factory dispatch, cache and workers execute production.
void ResourceLoader::RegisterGlobalResourceFactories(){}
std::shared_ptr<ResourceInitData> ResourceLoader::ReadResourceInitDataLegacy(const std::string& path,std::shared_ptr<File> file){
    file->BufferOffset=OTR_HEADER_SIZE;
    return ReadResourceInitDataBinary(path,std::make_shared<BinaryReader>(std::make_shared<MemoryStream>(file->Buffer)));
}
std::shared_ptr<tinyxml2::XMLDocument> ResourceLoader::CreateXMLReader(std::shared_ptr<File>,std::shared_ptr<ResourceInitData>){
    assert(false&&"binary disk-ownership fixture");return nullptr;
}
/* RESOURCE_LOADER */
/* RESOURCE_MANAGER */
}
/* RESOURCE_REGISTRY */
/* BASE_OWNER */
class MarkerResource:public Ship::Resource<std::string>{
  public:
    MarkerResource(std::shared_ptr<Ship::ResourceInitData> init,std::string value):Resource(std::move(init)),value(std::move(value)){}
    std::string* GetPointer() override{return &value;}
    size_t GetPointerSize() override{return value.size();}
    std::string value;
};
static std::function<void(const std::string&)> beforeRead;
class MarkerFactory:public Ship::ResourceFactory{
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,std::shared_ptr<Ship::ResourceInitData> init) override{
        if(beforeRead)beforeRead(init->Path);
        auto value=std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader)->ReadCString();
        if(!value.empty()&&value.back()==0)value.pop_back();
        return std::make_shared<MarkerResource>(init,std::move(value));
    }
    bool FileHasValidFormatAndReader(std::shared_ptr<Ship::File>,std::shared_ptr<Ship::ResourceInitData>) override{return true;}
};
static std::vector<char> Binary(const char* marker){
    std::vector<char> result(OTR_HEADER_SIZE,0);result[0]=char(Ship::Endianness::Little);
    const uint32_t type=0x54455354;std::memcpy(result.data()+4,&type,sizeof(type));
    result.insert(result.end(),marker,marker+std::strlen(marker)+1);return result;
}
static std::vector<char> Meta(const char* target){
    const std::string value=std::string("{\"path\":\"")+target+"\",\"format\":\"Binary\",\"type\":\"Marker\",\"version\":0}";
    std::vector<char> result(value.begin(),value.end());result.push_back(0);return result;
}
static std::string Value(const std::shared_ptr<Ship::IResource>& resource){
    return resource?std::static_pointer_cast<MarkerResource>(resource)->value:"missing";
}
int main(int argc,char** argv){
    const std::string scenario=argc>1?argv[1]:"root";
    auto stock=std::make_shared<Ship::MemoryArchive>("/game/soh.o2r");
    auto mod=std::make_shared<Ship::MemoryArchive>("/game/mods/sword.o2r");
    const char* root="objects/nei_gi_redesign/master_sword/gi_dl";
    const char* vertex="objects/nei_gi_redesign/master_sword/vertices";
    const char* texture="objects/nei_gi_redesign/master_sword/texture";
    for(const auto* path:{root,vertex,texture}){
        stock->Put(path,Binary("authored"));mod->Put(path,Binary("base custom"));
        mod->Put(std::string("alt/")+path,Binary("alt custom"));
    }
    stock->Put("stock_target",Binary("stock alias"));mod->Put("stock_target",Binary("mod target"));
    stock->Put("alias.meta",Meta("stock_target"));mod->Put("alias.meta",Meta("mod_target"));
    mod->Put("mod_target",Binary("mod alias"));mod->Put("missing_stock.meta",Meta("mod_target"));
    mod->Put("only_mod",Binary("mod-only resource"));stock->Put("delay",Binary("delayed stock"));
    Ship::mounted={stock,mod};auto owner=std::make_shared<Ship::ResourceManager>();
    owner->Init({}, {}, int32_t(std::thread::hardware_concurrency())-2);
    owner->GetResourceLoader()->RegisterResourceFactory(std::make_shared<MarkerFactory>(),RESOURCE_FORMAT_BINARY,"Marker",0x54455354,0);
    Ship::Context::active=owner;auto view=owner->CreateResourceView(stock);assert(view);
    if(scenario=="root"){
        assert(Value(view->LoadResource(root))=="authored"&&"production disk read ignored pinned archive");
        for(bool alt:{true,false,true,false,true}){
            owner->SetAltAssetsEnabled(alt);
            for(const char* path:{root,vertex,texture}){
                Fast::RenderResourceLookupStats stats;
                assert(Value(Fast::LoadRenderResource(*view,path,true,&stats))=="authored");
                assert(Value(view->LoadResource(std::hash<std::string>{}(path)))=="authored");
                assert(Value(owner->LoadResource(path))==(alt?"alt custom":"base custom"));
            }
            assert(!view->IsAltAssetsEnabled()&&owner->IsAltAssetsEnabled()==alt);
        }
        assert(!view->LoadResource("only_mod")&&!view->LoadResource("missing_stock"));
        assert(!owner->CreateResourceView(nullptr));
        std::cout<<"PASS real Ship cache/loader: pinned roots, vertices, textures, hashes, hot caches and independent donor toggles\n";
    }else if(scenario=="alias"){
        Ship::Context::active.reset();
        assert(Value(view->LoadResource("alias"))=="stock alias"&&"pinned .meta alias borrowed donor archive");
        Ship::Context::active=owner;
        assert(Value(owner->LoadResource("alias"))=="mod alias");assert(!view->LoadResource("missing_stock"));
        std::cout<<"PASS real .meta resolution confines alias and target to the shipped archive; ordinary donor priority preserved\n";
    }else if(scenario=="owner"){
        Ship::CrossRMRegistry::Register("oot",owner);
        assert(OOT_NeiEnsureGiBaseOwner()==1);
        auto registered=Ship::CrossRMRegistry::Get("oot-gi-base");
        assert(registered&&Value(registered->LoadResource(root))=="authored");
        assert(OOT_NeiEnsureGiBaseOwner()==1&&Ship::CrossRMRegistry::Get("oot-gi-base")==registered);
        Ship::CrossRMRegistry::Unregister("oot-gi-base");registered.reset();
        Ship::Context::shipped="/game/missing/soh.o2r";
        assert(OOT_NeiEnsureGiBaseOwner()==0&&!Ship::CrossRMRegistry::Get("oot-gi-base"));
        Ship::CrossRMRegistry::Unregister("oot");
        std::cout<<"PASS production base-owner initialization, reuse, registry teardown and missing shipped archive refusal\n";
    }else if(scenario=="lifetime"){
        std::promise<void> entered,release;auto released=release.get_future().share();
        beforeRead=[&](const std::string& path){if(path=="delay"){
            resourceLoadBlocked=true;entered.set_value();released.wait();resourceLoadBlocked=false;
        }};
        auto future=view->LoadResourceAsync("delay");entered.get_future().wait();
        auto dropping=std::async(std::launch::async,[view=std::move(view)]()mutable{view.reset();});
        assert(dropping.wait_for(std::chrono::milliseconds(100))==std::future_status::timeout&&"view teardown did not wait for its queued load");
        release.set_value();dropping.get();assert(Value(future.get())=="delayed stock");beforeRead={};
        std::cout<<"PASS view teardown waits for outstanding loads while donor workers remain alive\n";
    }else assert(false&&"unknown scenario");
    view.reset();Ship::Context::active.reset();owner.reset();Ship::mounted.clear();
}
