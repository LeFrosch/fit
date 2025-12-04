#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "winter.h"

static result_t
feature() {
    git_libgit2_init();

    git_repository* repo;
    try(fit_worktree(&repo));
    defer(git_repository_free, repo);

    git_reference* main;
    try(fit_branch_main(repo, &main));
    defer(git_reference_free, main);

    git_reference* branch;
    try(fit_branch_new(repo, "pull", main, &branch));
    defer(git_reference_free, branch);

    try(fit_checkout(repo, branch));

    return SUCCESS;
}

static result_t
dispatch(const int argc, char* const* argv) {
    (void)argc;

    if (strcmp(argv[1], "feature") == 0) {
        return feature();
    }

    // forward to git
    execvp("git", argv);
    failure(errno, msg("could not find git"));
}

int
main(const int argc, char* const* argv) {
    const int ret = dispatch(argc, argv);
    if (ret == SUCCESS) {
        return EXIT_SUCCESS;
    }

    for (uint32_t i = 0; i < error_trace_length(); i++) {
        error_frame_t* frame = error_trace_nth(i);
        fprintf(stderr, "%s @ %s:%d in %s\n", frame->msg, frame->file, frame->line, frame->func);
    }

    return EXIT_FAILURE;
}
