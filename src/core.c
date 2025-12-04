#include "core.h"
#include "uuid.h"
#include "winter.h"

#include <git2.h>
#include <limits.h>

#define check(expr)                                                                                                    \
    do {                                                                                                               \
        const int __ret = expr;                                                                                        \
        if (__ret != 0) {                                                                                              \
            const git_error* e = git_error_last();                                                                     \
            if (e != nullptr || e->message == nullptr) {                                                                                        \
                failure(__ret, msg("try %s", #expr), with_str(e->message));                                            \
            } else {                                                                                                   \
                failure(__ret, msg("try %s (unkown libgit error)", #expr));                                            \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

static const char* MAIN_BRANCH_REF = "refs/heads/main";
static const char* MASTER_BRANCH_REF = "refs/heads/master";

static const char* WORKTREE_NAME = "FIT_TEMP_WORKTREE";

result_t
fit_branch_main(git_repository* repo, git_reference** out) {
    int ret = git_reference_lookup(out, repo, MAIN_BRANCH_REF);
    if (ret == GIT_ENOTFOUND) {
        ret = git_reference_lookup(out, repo, MASTER_BRANCH_REF);
    }
    if (ret == 0) {
        return SUCCESS;
    }

    failure(ret, msg("could not find the main branch"));
}

static result_t
get_temp_path(char out[static PATH_MAX]) {
    uuid_t uuid;
    try(uuid_v7_create(uuid));

    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);

    snprintf(out, PATH_MAX, "/tmp/%s", uuid_str);

    return SUCCESS;
}

result_t
fit_worktree(git_repository** out) {
    git_repository* repo;
    check(git_repository_open(&repo, "."));
    defer(git_repository_free, repo);

    git_worktree* worktree;
    const int ret = git_worktree_lookup(&worktree, repo, WORKTREE_NAME);

    if (ret == GIT_ENOTFOUND) {
        git_worktree_add_options opts;
        check(git_worktree_add_options_init(&opts, GIT_WORKTREE_ADD_OPTIONS_VERSION));

        char path[PATH_MAX];
        try(get_temp_path(path));

        check(git_worktree_add(&worktree, repo, WORKTREE_NAME, path, &opts));
    } else if (ret != 0) {
        failure(ret, msg("worktree lookup failed"));
    }

    defer(git_worktree_free, worktree);
    check(git_repository_open_from_worktree(out, worktree));

    return SUCCESS;
}

result_t
fit_branch_new(git_repository* repo, const char* prefix, const git_reference* ref, git_reference** out) {
    uuid_t uuid;
    try(uuid_v7_create(uuid));

    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);

    char branch_name[64];
    snprintf(branch_name, sizeof(branch_name), "%s/%s", prefix, uuid_str);

    git_object* obj;
    check(git_reference_peel(&obj, ref, GIT_OBJ_COMMIT));
    defer(git_object_free, obj);

    git_commit* commit;
    check(git_commit_lookup(&commit, repo, git_object_id(obj)));
    defer(git_commit_free, commit);

    check(git_branch_create(out, repo, branch_name, commit, 0));

    return SUCCESS;
}

result_t
fit_checkout(git_repository* repo, const git_reference* ref) {
    git_object* obj;
    check(git_reference_peel(&obj, ref, GIT_OBJ_COMMIT));
    defer(git_object_free, obj);

    git_checkout_options opts;
    check(git_checkout_init_options(&opts, GIT_CHECKOUT_OPTIONS_VERSION));

    opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    check(git_checkout_tree(repo, obj, &opts));
    check(git_repository_set_head(repo, git_reference_name(ref)));

    return SUCCESS;
}
