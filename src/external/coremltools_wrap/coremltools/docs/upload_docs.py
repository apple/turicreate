import argparse
import readme_session
import functools
import coremltools
import pathlib
import os
import re

parser = argparse.ArgumentParser(description="Upload docs to ReadMe.")
parser.add_argument(
    "--version",
    type=str,
    help="Version to upload.",
    default=re.split("[a-z]+", coremltools.version.__version__)[0],
)
parser.add_argument(
    "--from_source_version",
    type=str,
    help="Create a version from this version if current CMLT version does not have docs."
    + "Default is the most recent version",
    default=None,
)
parser.add_argument(
    "--release_version", action="store_true", help="Release the version to the public."
)
parser.add_argument(
    "--set_version_stable",
    action="store_true",
    help="Set this version as the stable (main) version.",
)
parser.add_argument("--auth_token", type=str, help="Token for authentication.")

args = parser.parse_args()


# Remove "coremltools" from the beginning of of all filenames in this path
def sanitize_names(path):
    if os.path.isdir(path):
        # get all subdirs in path and recursively transfer all files in that subdir
        subdirpath = path
        onlydirs = [
            f
            for f in os.listdir(subdirpath)
            if os.path.isdir(os.path.join(subdirpath, f))
        ]
        for dir in onlydirs:
            sanitize_names(os.path.join(path, dir))

    # get all filenames in current dir
    files = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]

    # iterate through all filenames and remove coremltools prefix
    for file in files:
        if file.startswith("coremltools"):
            currpath = os.path.join(path, file)
            newpath = os.path.join(path, file[file.find(".") + 1 :])
            os.rename(currpath, newpath)


# API Setup
sess = readme_session.ReadMeSession(args.auth_token)

# Create version
if args.version not in sess.get_versions():
    sess.create_version(args.version, args.from_source_version)
sess.set_api_version(args.version)

# Upload generated folders
docspath = str(pathlib.Path(__file__).parent.absolute() / "_build" / "html")
dirs = [
    os.path.join(docspath, f)
    for f in os.listdir(docspath)
    if os.path.isdir(os.path.join(docspath, f))
]
for thisdir in dirs:
    if os.path.basename(thisdir)[0] is not "_":
        sanitize_names(thisdir)
        print("--------- Processing " + thisdir + " ----------")
        category = os.path.basename(thisdir)
        sess.empty_category(category)
        sess.upload(path=thisdir, category=category, recursive=True)
        print("-------------------- Done ---------------------\n")

# Release the version or set it to stable
if args.release_version or args.set_version_stable:
    sess.update_version(
        version,
        is_stable=args.set_version_stable or sess.get_version()["is_stable"],
        is_hidden=not args.release_version or not sess.get_version()["is_hidden"],
    )
