#pragma once

#include "winter.h"

#include <git2.h>

result_t
fit_worktree(git_repository** out);

result_t
fit_branch_new(git_repository* repo, const char* prefix, const git_reference* ref, git_reference** out);

result_t
fit_branch_main(git_repository* repo, git_reference** out);

result_t
fit_checkout(git_repository* repo, const git_reference* ref);

defer_impl(git_repository_free) {
    defer_guard();
    git_repository_free(*defer_arg(git_repository*));
}

defer_impl(git_object_free) {
    defer_guard();
    git_object_free(*defer_arg(git_object*));
}

defer_impl(git_commit_free) {
    defer_guard();
    git_commit_free(*defer_arg(git_commit*));
}

defer_impl(git_reference_free) {
    defer_guard();
    git_reference_free(*defer_arg(git_reference*));
}

defer_impl(git_worktree_free) {
    defer_guard();
    git_worktree_free(*defer_arg(git_worktree*));
}
