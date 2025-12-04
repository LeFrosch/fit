#pragma once

#include "winter.h"

#include <git2.h>

#define check(expr)                                                                                                    \
    do {                                                                                                               \
        const int __ret = expr;                                                                                        \
        if (__ret != 0) {                                                                                              \
            const git_error* e = git_error_last();                                                                     \
            if (e != nullptr || e->message == nullptr) {                                                               \
                failure(__ret, msg("try %s", #expr), with_str(e->message));                                            \
            } else {                                                                                                   \
                failure(__ret, msg("try %s (unkown libgit error)", #expr));                                            \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

result_t
fit_init(git_repository** out);

result_t
fit_worktree(git_repository* repo, git_repository** out);

result_t
fit_branch_new(git_repository* repo, const char* prefix, const git_reference* ref, git_reference** out);

result_t
fit_branch_main(git_repository* repo, git_reference** out);

result_t
fit_checkout(git_repository* repo, const git_reference* ref);

result_t
fit_cherry_pick(git_repository* repo, const git_reference* ref);

result_t
fit_push(git_repository* repo);

void
fit_shutdown(git_repository* repo);

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

defer_impl(git_index_free) {
    defer_guard();
    git_index_free(*defer_arg(git_index*));
}

defer_impl(git_remote_free) {
    defer_guard();
    git_remote_free(*defer_arg(git_remote*));
}

defer_impl(git_signature_free) {
    defer_guard();
    git_signature_free(*defer_arg(git_signature*));
}

defer_impl(git_tree_free) {
    defer_guard();
    git_tree_free(*defer_arg(git_tree*));
}

defer_impl(fit_shutdown) {
    defer_guard();
    fit_shutdown(*defer_arg(git_repository*));
}
