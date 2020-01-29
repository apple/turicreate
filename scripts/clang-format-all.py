#!/usr/bin/env python3
import os
import sys
import re
import copy
import argparse
import subprocess
import threading
from queue import Queue


SCRIPT_DIR = os.path.dirname(__file__)
WORKSPACE = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
TAKS_SIZE = 10
CXX_SUFFIX_REGEX = re.compile(r"[\w/]+\.(c|cc|cxx|cpp|h|hpp|hxx|m|mm)$")
NUM_THREADS = 2


def producer(start, queue, match, num_threads):
    """push to task by directory level"""
    try:
        task = []
        for dirname, _, basenames in os.walk(start):
            for basename in basenames:
                fname = os.path.join(dirname, basename)
                if (
                    CXX_SUFFIX_REGEX.match(fname)
                    and match["include"](fname)
                    and not match["exclude"](fname)
                ):
                    task.append(fname)
            queue.put(task)
            task = []

        if task:
            queue.put(task)
    except Exception as e:
        print(e)
        raise e
    finally:
        print("producer stops all consumer threads")
        for _ in range(NUM_THREADS):
            queue.put(None)

        queue.join()


def consumer(queue, do_work):
    while True:
        item = queue.get(timeout=100)
        if item is None:
            queue.task_done()
            break
        try:
            do_work(item)
        except Exception as e:
            print(e)
            raise e
        finally:
            # tell the queue to unblock
            queue.task_done()


def get_parser():
    parser = argparse.ArgumentParser(
        description="format all cpp files recursively in a directory"
    )

    # exclude regex
    parser.add_argument(
        "--path", default=WORKSPACE, type=str, help="path to start the search"
    )

    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="list files wihtout invoking clang-format",
    )

    parser.add_argument(
        "--cfmt-args",
        default=["style=google", "i"],
        nargs="+",
        help="args for clang-format, without any '-' in options",
    )

    # regex and glob, only one
    parser.add_argument(
        "--exclude-regex", type=str, help="files to exclude by regex pattern"
    )

    parser.add_argument(
        "--include-regex", type=str, help="files to exclude by regex pattern"
    )

    return parser


def transform_clang_format_args(args):
    for i, arg in enumerate(args):
        pos = args[i].find("=")
        if pos != -1:
            if pos == 0 or pos == len(args) - 1:
                raise ValueError("Invalid option '%s'" % args[i])
            else:
                args[i] = "--" + arg
        else:
            args[i] = "-" + arg


def dispatch_to_threads(args):
    if not args.path:
        raise ValueError("path cannot be empty")

    if not os.path.isdir(args.path):
        ws_path = os.path.join(WORKSPACE, args.path)
        if not os.path.isdir(ws_path):
            raise ValueError("path %s doesn't exist" % args.path)
        else:
            args.path = ws_path

    match = {"include": lambda x: True, "exclude": lambda x: False}

    if args.include_regex:
        match["include"] = re.compile(args.include_regex)

    if args.exclude_regex:
        match["exclude"] = re.compile(args.exclude_regex)

    cmd = []
    if not args.dry_run:
        cmd.append("clang-format")
        if args.cfmt_args:
            transform_clang_format_args(args.cfmt_args)
            cmd.extend(args.cfmt_args)
        # define a closure
        def do_work(files):
            my_cmd = copy.deepcopy(cmd)
            my_cmd.extend(files)
            try:
                subprocess.check_output(my_cmd)
            except subprocess.CalledProcessError as e:
                print(e.output)
                print(cmd, files)

    else:
        # not thread safe
        do_work = lambda x: print("cmd:", cmd, ", files:", x)

    qu = Queue()

    my_threads = []

    th_prod = threading.Thread(
        target=producer, args=(args.path, qu, match, NUM_THREADS)
    )
    th_prod.start()
    my_threads.append(th_prod)

    for _ in range(NUM_THREADS):
        th_con = threading.Thread(target=consumer, args=(qu, do_work))
        th_con.start()
        my_threads.append(th_con)

    assert NUM_THREADS == len(my_threads) - 1

    for t in my_threads:
        t.join()

    print("%d threads finish running" % NUM_THREADS)


if __name__ == "__main__":
    parser = get_parser()
    args = parser.parse_args()
    dispatch_to_threads(args)
