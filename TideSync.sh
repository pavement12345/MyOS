#!/usr/bin/env bash
set -Eeuo pipefail

REMOTE="${TIDE_REMOTE:-origin}"
INTERVAL="${TIDE_SYNC_INTERVAL:-60}"
MAX_RETRIES="${TIDE_SYNC_RETRIES:-3}"
BOT_NAME="${TIDE_BOT_NAME:-Tide Sync Bot}"
BOT_EMAIL="${TIDE_BOT_EMAIL:-tide-sync-bot@users.noreply.github.com}"

log() {
    local msg="[$(date -u '+%Y-%m-%d %H:%M:%S UTC')] [Tide Sync] $*"
    printf '%s\n' "$msg"
    printf '%s\n' "$msg" >> "$LOG_FILE"
}

fail() {
    log "ERROR: $*"
    exit 1
}

command -v git >/dev/null 2>&1 || {
    echo "Git is required." >&2
    exit 1
}

ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" ||
    fail "Run inside the Tide repository."

cd "$ROOT"

BRANCH="$(git symbolic-ref --quiet --short HEAD)" ||
    fail "Detached HEAD is not supported."

GIT_DIR="$(git rev-parse --git-dir)"
[[ "$GIT_DIR" = /* ]] || GIT_DIR="$ROOT/$GIT_DIR"

LOG_FILE="$GIT_DIR/tide-sync.log"
LOCK_DIR="$GIT_DIR/tide-sync.lock"

STOP=0
LOCKED=0

[[ "$INTERVAL" =~ ^[0-9]+$ ]] && (( INTERVAL >= 5 )) ||
    fail "Sync interval must be at least 5 seconds."

[[ "$MAX_RETRIES" =~ ^[0-9]+$ ]] && (( MAX_RETRIES >= 1 )) ||
    fail "Retries must be at least 1."

git remote get-url "$REMOTE" >/dev/null 2>&1 ||
    fail "Remote '$REMOTE' is missing."

UPSTREAM="$(git rev-parse --abbrev-ref --symbolic-full-name '@{upstream}' 2>/dev/null || true)"

if [[ -z "$UPSTREAM" ]]; then
    UPSTREAM="$REMOTE/$BRANCH"
fi

UPSTREAM_REMOTE="${UPSTREAM%%/*}"
UPSTREAM_BRANCH="${UPSTREAM#*/}"

[[ "$UPSTREAM_REMOTE" == "$REMOTE" ]] ||
    fail "Branch tracks a different remote."

mkdir "$LOCK_DIR" 2>/dev/null ||
    fail "Another Tide Sync process may already be running."

LOCKED=1

cleanup() {
    if (( LOCKED )); then
        rmdir "$LOCK_DIR" 2>/dev/null || true
    fi
}

shutdown() {
    STOP=1
    log "Shutdown requested."
}

trap cleanup EXIT
trap shutdown INT TERM

log "========================================"
log "TIDE SYNC BOT STARTED"
log "Repository: $ROOT"
log "Branch: $BRANCH"
log "Upstream: $UPSTREAM"
log "Interval: ${INTERVAL}s"
log "========================================"

retry_fetch() {
    local attempt=1

    while (( attempt <= MAX_RETRIES )); do
        log "Fetching remote ($attempt/$MAX_RETRIES)."

        if git fetch --prune "$REMOTE"; then
            return 0
        fi

        if (( attempt < MAX_RETRIES )); then
            log "Fetch failed. Retrying in 5 seconds."
            sleep 5
        fi

        ((attempt += 1))
    done

    return 1
}

check_git_state() {
    if [[ -f "$(git rev-parse --git-path MERGE_HEAD)" ]] ||
       [[ -d "$(git rev-parse --git-path rebase-merge)" ]] ||
       [[ -d "$(git rev-parse --git-path rebase-apply)" ]] ||
       [[ -f "$(git rev-parse --git-path CHERRY_PICK_HEAD)" ]]; then
        log "Unfinished Git operation detected."
        log "Manual intervention required."
        return 1
    fi

    return 0
}

commit_changes() {
    git add -A

    if ! git diff --cached --quiet; then
        local message
        message="Tide Sync: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"

        log "Committing local changes."

        git -c user.name="$BOT_NAME" \
            -c user.email="$BOT_EMAIL" \
            commit -m "$message" ||
            return 1

        log "Commit created: $message"
    else
        log "No local changes."
    fi
}

create_backup() {
    local stamp backup suffix=0

    stamp="$(date -u '+%Y%m%d-%H%M%S')"
    backup="backup/${BRANCH}-${stamp}"

    while git show-ref --verify --quiet "refs/heads/$backup"; do
        ((suffix += 1))
        backup="backup/${BRANCH}-${stamp}-${suffix}"
    done

    git branch "$backup" HEAD || return 1

    log "Recovery branch created: $backup"
}

merge_remote() {
    log "Merging $UPSTREAM."

    if git merge --no-edit "$UPSTREAM"; then
        log "Merge successful."
        return 0
    fi

    log "Merge failed. Attempting safe merge abort."

    if git rev-parse --verify MERGE_HEAD >/dev/null 2>&1; then
        git merge --abort ||
            log "WARNING: Merge abort failed. Inspect repository manually."
    fi

    log "Manual conflict resolution required."
    return 1
}

get_counts() {
    local counts
    counts="$(git rev-list --left-right --count "HEAD...$UPSTREAM")"
    read -r LOCAL_AHEAD REMOTE_AHEAD <<< "$counts"
}

sync_once() {
    check_git_state || return 2

    commit_changes || {
        log "Local commit failed."
        return 2
    }

    retry_fetch || {
        log "Fetch failed after all retries."
        return 1
    }

    git rev-parse --verify "$UPSTREAM^{commit}" >/dev/null 2>&1 || {
        log "Remote branch $UPSTREAM does not exist."
        return 2
    }

    get_counts

    if (( REMOTE_AHEAD > 0 )); then
        log "Incoming commits: $REMOTE_AHEAD"

        create_backup || {
            log "Backup creation failed."
            return 2
        }

        merge_remote || return 2
    else
        log "No incoming commits."
    fi

    get_counts

    if (( LOCAL_AHEAD == 0 )); then
        log "Already synchronized."
        return 0
    fi

    local attempt=1

    while (( attempt <= MAX_RETRIES )); do
        log "Pushing $LOCAL_AHEAD commits ($attempt/$MAX_RETRIES)."

        if git push "$REMOTE" "HEAD:refs/heads/$UPSTREAM_BRANCH"; then
            log "Push successful."
            return 0
        fi

        if (( attempt == MAX_RETRIES )); then
            break
        fi

        log "Push rejected or failed. Refreshing remote."

        retry_fetch || return 1

        if ! merge_remote; then
            return 2
        fi

        get_counts

        if (( LOCAL_AHEAD == 0 )); then
            log "Remote already contains local changes."
            return 0
        fi

        sleep 5
        ((attempt += 1))
    done

    log "Push failed after all retries."
    return 1
}

while (( ! STOP )); do
    log "----------------------------------------"
    log "Starting synchronization cycle."

    STATUS=0
    sync_once || STATUS=$?

    case "$STATUS" in
        0) log "Synchronization completed successfully." ;;
        1) log "Temporary failure. Will retry next cycle." ;;
        2) log "Manual intervention required. Skipping cycle." ;;
        *) log "Unexpected synchronization status: $STATUS" ;;
    esac

    if (( ! STOP )); then
        log "Next synchronization in ${INTERVAL} seconds."
        sleep "$INTERVAL" || true
    fi
done

log "Tide Sync stopped."
