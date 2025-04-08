#!/bin/sh -ex
# Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@suse.com>

if [ -z "$PATCHWORK_URL" ]; then
        PATCHWORK_URL="https://patchwork.ozlabs.org"
fi

if [ -z "$PATCHWORK_SINCE" ]; then
        PATCHWORK_SINCE=3600
fi

fetch_series() {
        local current_time=$(date +%s)
        local since_time=$(expr $current_time - $PATCHWORK_SINCE)
        local date=$(date -u -d @$since_time +"%Y-%m-%dT%H:%M:%SZ")

        curl -k -G "$PATCHWORK_URL/api/events/" \
                --data "category=series-completed" \
                --data "project=ltp" \
                --data "state=new" \
                --data "since=$date" \
                --data "archive=no" |
                jq -r '.[] | "\(.payload.series.id):\(.payload.series.mbox)"'
}

get_patches() {
        local series_id="$1"

        curl -k -G "$PATCHWORK_URL/api/patches/" \
                --data "project=ltp" \
                --data "series=$series_id" |
                jq -r '.[] | "\(.id)"'
}

set_patch_state() {
        local patch_id="$1"
        local state="$2"

        curl -k -X PATCH \
                -H "Authorization: Token $PATCHWORK_TOKEN" \
                -F "state=$state" \
                "$PATCHWORK_URL/api/patches/$patch_id/"
}

set_series_state() {
        local series_id="$1"
        local state="$2"

        get_patches "$series_id" | while IFS= read -r patch_id; do
                if [ -n "$patch_id" ]; then
                        set_patch_state "$patch_id" "$state"
                fi
        done
}

get_checks() {
        local patch_id="$1"

        curl -k -G "$PATCHWORK_URL/api/patches/$patch_id/checks/" |
                jq -r '.[] | "\(.id)"'
}

already_tested() {
        local series_id="$1"

        get_patches "$series_id" | while IFS= read -r patch_id; do
                if [ ! -n "$patch_id" ]; then
                        continue
                fi

                get_checks "$patch_id" | while IFS= read -r check_id; do
                        if [ -n "$check_id" ]; then
                                echo "$check_id"
                                return
                        fi
                done
        done
}

verify_new_patches() {
        local output="output.txt"

        fetch_series | while IFS=: read -r series_id series_mbox; do
                if [ ! -n "$series_id" ]; then
                        continue
                fi

                tested=$(already_tested "$series_id")
                if [ -n "$tested" ]; then
                        continue
                fi

                echo -n "$series_id:$series_mbox;" >>"$output"
        done

        if [ -e "$output" ]; then
                cat "$output"
        fi
}

apply_series() {
        local series_id="$1"
        local series_mbox="$2"

        git config --global user.name 'GitHub CI'
        git config --global user.email 'patchwork.tester@example.org'
        git config --global --add safe.directory $GITHUB_WORKSPACE

        curl -k "$series_mbox" | git am

        # we set patch state after applying it, so
        # the next triggering won't take patch into account
        set_series_state "$series_id" "needs-review-ack"
}

send_results() {
        local series_id="$1"
        local target_url="$2"

        local context=$(echo "$3" | sed 's/:/_/g; s/\//_/g')
        if [ -n "$CC" ]; then
                context="${context}_${CC}"
        fi

        if [ -n "$ARCH" ]; then
                context="${context}_${ARCH}"
        fi

        local result="$4"
        if [ "$result" = "cancelled" ]; then
                return
        fi

        local state="fail"
        if [ "$result" = "success" ]; then
                state="success"
        fi

        get_patches "$series_id" | while IFS= read -r patch_id; do
                if [ -n "$patch_id" ]; then
                        curl -k -X POST \
                                -H "Authorization: Token $PATCHWORK_TOKEN" \
                                -F "state=$state" \
                                -F "context=$context" \
                                -F "target_url=$target_url" \
                                -F "description=$result" \
                                "$PATCHWORK_URL/api/patches/$patch_id/checks/"
                fi
        done
}

run="$1"

if [ -z "$run" -o "$run" = "verify" ]; then
        verify_new_patches
elif [ -z "$run" -o "$run" = "apply" ]; then
        apply_series "$2" "$3"
elif [ -z "$run" -o "$run" = "check" ]; then
        send_results "$2" "$3" "$4" "$5"
else
        echo "Available commands: apply, check, verify"
        exit 1
fi
