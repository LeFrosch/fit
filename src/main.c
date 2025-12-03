#include <git2.h>
#include <stdio.h>

int main() {
    git_libgit2_init();
    
    printf("Libgit2 initialized!\n");

    git_libgit2_shutdown();
    return 0;
}
