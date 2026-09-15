#include "../../main/game_2048_storage.h"
#include "nvs.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static std::vector<unsigned char> blob, pending;
static int failure, init_error;
static unsigned handles;
esp_err_t nvs_flash_init() { return init_error; }
esp_err_t nvs_open(const char *, int mode, nvs_handle_t *h) {
    if(failure==1) return 9;
    if(mode==NVS_READONLY && blob.empty())return ESP_ERR_NVS_NOT_FOUND;
    *h=++handles;return 0;
}
esp_err_t nvs_get_blob(nvs_handle_t, const char *, void *data, size_t *size) {
    if(failure==2)return 9;
    if(!data){*size=blob.size();return 0;}
    if(*size<blob.size())return 9;
    std::memcpy(data,blob.data(),blob.size());*size=blob.size();return 0;
}
esp_err_t nvs_set_blob(nvs_handle_t,const char *,const void *data,size_t size) {
    if(failure==3)return 9;
    auto p=static_cast<const unsigned char *>(data);pending.assign(p,p+size);return 0;
}
esp_err_t nvs_commit(nvs_handle_t) { if(failure==4)return 9;blob=pending;return 0; }
void nvs_close(nvs_handle_t) { --handles; }
static void check(bool b,const char *msg){if(!b){std::fprintf(stderr,"FAIL: %s\n",msg);std::exit(2);}}
int main(int argc,char **) {
    if(argc>1){
        init_error=9;
        check(!game2048_storage::initialize(),"init failure reported");
        check(!game2048_storage::save(game2048::Game().archive()),"unavailable storage rejects save");
        std::puts("PASS: NVS init failure remains non-destructive");return 0;
    }
    check(game2048_storage::initialize(),"initialize");
    game2048::Archive loaded;
    check(!game2048_storage::load(loaded),"fresh storage has no save");
    game2048::Game game(123);game.move(game2048::Direction::Left);
    check(game2048_storage::save(game.archive()),"save");
    check(game2048_storage::load(loaded),"load");
    check(loaded.current.cells==game.archive().current.cells && loaded.current.random==game.archive().current.random,"round trip state");
    auto original=blob;
    blob.back()^=1;check(!game2048_storage::load(loaded),"CRC corruption rejected");
    blob=original;blob[0]^=1;check(!game2048_storage::load(loaded),"bad magic rejected");
    blob=original;blob[4]^=1;check(!game2048_storage::load(loaded),"bad schema rejected");
    blob=original;blob.pop_back();check(!game2048_storage::load(loaded),"wrong size rejected");
    blob=original;
    for(failure=1;failure<=4;++failure){
        if(failure<=2)check(!game2048_storage::load(loaded),"read failure reported");
        if(failure!=2)check(!game2048_storage::save(game.archive()),"write failure reported");
        check(handles==0,"handle closed on failure");
    }
    check(blob==original,"failed writes retain committed save");
    std::puts("PASS: NVS round trip, CRC/schema/size validation, read/write errors, handle cleanup");
}
