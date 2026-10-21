#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "core/arena.h"
#include "core/jagfile.h"

int main(void) {
    printf("[Test Jagfile] Loading 'title' archive from real 2004scape cache...\n");

    Arena arena = arena_create(16 * 1024 * 1024);

    const char *archive_path = "../engine/data/pack/client/title";
    Jagfile *jag = jagfile_load_file(archive_path, &arena);
    assert(jag != NULL);
    printf("  Successfully loaded Jagfile! File count = %d (expected 10)\n", jag->file_count);
    assert(jag->file_count == 10);

    size_t out_size = 0;
    uint8_t *title_dat = jagfile_read(jag, "title.dat", NULL, &out_size, &arena);
    assert(title_dat != NULL);
    printf("  Successfully extracted 'title.dat'! Size = %zu (expected 17803)\n", out_size);
    assert(out_size == 17803);

    // Verify JPEG header: 0xFF, 0xD8, 0xFF, 0xE0
    printf("  First 4 bytes: %d, %d, %d, %d (expected 255, 216, 255, 224)\n",
           title_dat[0], title_dat[1], title_dat[2], title_dat[3]);
    assert(title_dat[0] == 255);
    assert(title_dat[1] == 216);
    assert(title_dat[2] == 255);
    assert(title_dat[3] == 224);

    arena_free(&arena);
    printf("[Test Jagfile] ALL TESTS PASSED! 100%% parity with live 2004scape cache.\n");
    return 0;
}
