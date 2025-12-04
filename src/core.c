#include "core.h"
#include "uuid.h"
#include "winter.h"

#include <git2.h>
#include <limits.h>
#include <string.h>

static const char* WORKTREE_NAME = "FIT_TEMP_WORKTREE";

result_t
fit_init(git_repository** out) {
    git_libgit2_init();

    check(git_repository_open(out, "."));
    return SUCCESS;
}

result_t
fit_branch_main(git_repository* repo, git_reference** out) {
    int ret = git_reference_lookup(out, repo, "refs/remotes/origin/main");
    if (ret == GIT_ENOTFOUND) {
        ret = git_reference_lookup(out, repo, "refs/remotes/origin/master");
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
fit_worktree(git_repository* repo, git_repository** out) {
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

result_t
fit_cherry_pick(git_repository* repo, const git_reference* ref) {
    // cherry-pick the changes to the branch
    git_object* obj;
    check(git_reference_peel(&obj, ref, GIT_OBJ_COMMIT));
    defer(git_object_free, obj);

    git_commit* commit;
    check(git_commit_lookup(&commit, repo, git_object_id(obj)));
    defer(git_commit_free, commit);

    char oid[130] = { 0 };
    git_oid_fmt(oid, git_reference_target(ref));

    printf("cherry-pick %.7s %s\n", oid, git_commit_message(commit));

    git_cherrypick_options opts;
    check(git_cherrypick_init_options(&opts, GIT_CHERRYPICK_OPTIONS_VERSION));
    check(git_cherrypick(repo, commit, &opts));

    // check for conflicts
    git_index* index;
    check(git_repository_index(&index, repo));
    defer(git_index_free, index);

    if (git_index_has_conflicts(index)) {
        failure(EIO, msg("conflicts after cherry-pick"));
    }

    // create a new commit from the changes
    git_oid tree_oid;
    check(git_index_write_tree(&tree_oid, index));

    git_tree* tree;
    check(git_tree_lookup(&tree, repo, &tree_oid));
    defer(git_tree_free, tree);

    git_reference* head;
    check(git_repository_head(&head, repo));
    defer(git_reference_free, head);

    git_commit* head_commit;
    check(git_reference_peel((git_object**)&head_commit, head, GIT_OBJ_COMMIT));
    defer(git_commit_free, head_commit);

    git_signature* signature;
    check(git_signature_default(&signature, repo));
    defer(git_signature_free, signature);

    git_oid new_commit_oid;
    check(git_commit_create(
      &new_commit_oid,
      repo,
      "HEAD",
      git_commit_author(commit),
      signature,
      NULL,
      git_commit_message(commit),
      tree,
      1,
      (const git_commit**)&head_commit
    ));

    return SUCCESS;
}

static int
acquire_ssh_credentials(
  git_cred** out,
  const char* url,
  const char* username,
  const unsigned int allowed_types,
  void* payload
) {
    (void)url;
    (void)payload;

    if (allowed_types & GIT_CREDTYPE_SSH_KEY) {
        return git_cred_ssh_key_from_agent(out, username);
    } else {
        return -1;
    }
}

result_t
fit_push(git_repository* repo) {
    git_remote* remote;
    check(git_remote_lookup(&remote, repo, "origin"));
    defer(git_remote_free, remote);

    git_push_options opts;
    check(git_push_init_options(&opts, GIT_PUSH_OPTIONS_VERSION));
    opts.callbacks.credentials = acquire_ssh_credentials;

    git_reference* head;
    git_repository_head(&head, repo);
    defer(git_reference_free, head);

    git_object* obj;
    check(git_reference_peel(&obj, head, GIT_OBJ_COMMIT));
    defer(git_object_free, obj);

    git_commit* commit;
    check(git_commit_lookup(&commit, repo, git_object_id(obj)));
    defer(git_commit_free, commit);

    char oid[130] = { 0 };
    git_oid_fmt(oid, git_reference_target(head));

    const char* ref = git_reference_name(head);

    char refspec[512] = { 0 };
    snprintf(refspec, sizeof(refspec), "%s:%s", ref, ref);

    const git_strarray refspecs = { (char*[]){ refspec }, 1 };
    check(git_remote_push(remote, &refspecs, &opts));

    return SUCCESS;
}

void
fit_shutdown(git_repository* repo) {
    git_repository_free(repo);
    git_libgit2_shutdown();
}
