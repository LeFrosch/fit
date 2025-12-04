#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "winter.h"

static result_t
feature() {
    git_repository* repo;
    try(fit_init(&repo));
    defer(fit_shutdown, repo);

    git_repository* worktree;
    try(fit_worktree(repo, &worktree));
    defer(git_repository_free, worktree);

    git_reference* main;
    try(fit_branch_main(worktree, &main));
    defer(git_reference_free, main);

    git_reference* branch;
    try(fit_branch_new(worktree, "pull", main, &branch));
    defer(git_reference_free, branch);

    try(fit_checkout(worktree, branch));

    git_reference* commit;
    try(git_repository_head(&commit, repo));
    try(fit_cherry_pick(worktree, commit));

    try(fit_push(worktree));

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
