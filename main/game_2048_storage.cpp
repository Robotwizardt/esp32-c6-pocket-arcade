#include "game_2048_storage.h"

#include <cstring>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace game2048_storage {
namespace {

constexpr const char *kTag = "game_2048_store";
constexpr const char *kNamespace = "arcade";
constexpr const char *kKey = "2048";
constexpr uint32_t kMagic = 0x32303438u;  // ASCII "2048"
constexpr uint16_t kSchema = 1;

struct Blob {
    uint32_t magic;
    uint16_t schema;
    uint16_t payload_size;
    uint32_t crc32;
    game2048::Archive archive;
};

static_assert(sizeof(game2048::Archive) <= UINT16_MAX,
              "The archive size must fit the storage envelope header");

bool g_attempted = false;
bool g_ready = false;

uint32_t crc32(const void *data, size_t size)
{
    const auto *bytes = static_cast<const uint8_t *>(data);
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t index = 0; index < size; ++index) {
        crc ^= bytes[index];
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

void report_unavailable(const char *operation, esp_err_t error)
{
    ESP_LOGW(kTag, "%s unavailable (%s); keeping the game in RAM",
             operation, esp_err_to_name(error));
}

}  // namespace

bool initialize()
{
    if (g_attempted) return g_ready;
    g_attempted = true;

    const esp_err_t error = nvs_flash_init();
    if (error == ESP_OK) {
        g_ready = true;
        return true;
    }

    // A full or old partition is intentionally left untouched.  Erasing NVS
    // here would remove unrelated user settings and make recovery destructive.
    ESP_LOGE(kTag, "NVS init failed (%s); no erase performed, saves stay in RAM",
             esp_err_to_name(error));
    return false;
}

bool load(game2048::Archive &archive)
{
    if (!g_ready) return false;

    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (error != ESP_OK) {
        if (error != ESP_ERR_NVS_NOT_FOUND) report_unavailable("NVS load", error);
        return false;
    }

    size_t size = 0;
    error = nvs_get_blob(handle, kKey, nullptr, &size);
    if (error != ESP_OK) {
        nvs_close(handle);
        if (error != ESP_ERR_NVS_NOT_FOUND) report_unavailable("NVS load", error);
        return false;
    }
    if (size != sizeof(Blob)) {
        ESP_LOGW(kTag, "Ignoring 2048 save with unexpected size %u", static_cast<unsigned>(size));
        nvs_close(handle);
        return false;
    }

    Blob blob{};
    size = sizeof(blob);
    error = nvs_get_blob(handle, kKey, &blob, &size);
    nvs_close(handle);
    if (error != ESP_OK || size != sizeof(blob)) {
        if (error != ESP_ERR_NVS_NOT_FOUND) report_unavailable("NVS load", error);
        return false;
    }
    if (blob.magic != kMagic || blob.schema != kSchema ||
        blob.payload_size != sizeof(game2048::Archive) ||
        blob.crc32 != crc32(&blob.archive, sizeof(blob.archive))) {
        ESP_LOGW(kTag, "Ignoring invalid or incompatible 2048 save");
        return false;
    }

    archive = blob.archive;
    ESP_LOGI(kTag, "Loaded 2048 save: score=%lu best=%lu",
             static_cast<unsigned long>(archive.current.score),
             static_cast<unsigned long>(archive.best));
    return true;
}

bool save(const game2048::Archive &archive)
{
    if (!g_ready) return false;

    Blob blob{};
    blob.magic = kMagic;
    blob.schema = kSchema;
    blob.payload_size = static_cast<uint16_t>(sizeof(game2048::Archive));
    blob.archive = archive;
    blob.crc32 = crc32(&blob.archive, sizeof(blob.archive));

    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (error != ESP_OK) {
        report_unavailable("NVS save", error);
        return false;
    }
    error = nvs_set_blob(handle, kKey, &blob, sizeof(blob));
    if (error == ESP_OK) error = nvs_commit(handle);
    nvs_close(handle);
    if (error != ESP_OK) {
        report_unavailable("NVS save", error);
        return false;
    }
    ESP_LOGI(kTag, "Saved 2048 save: score=%lu best=%lu",
             static_cast<unsigned long>(archive.current.score),
             static_cast<unsigned long>(archive.best));
    return true;
}

}  // namespace game2048_storage
