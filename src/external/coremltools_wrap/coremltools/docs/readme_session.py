import json
import os
from requests.auth import HTTPBasicAuth
import requests

_readme_api_url = "https://dash.readme.io/api/v1/"


class ReadMeSession:
    # Passed for every API call
    __headers = {"Accept": "application/json", "content-type": "application/json"}

    # Map: <version string -> version info>
    __versions = None

    def __init__(self, auth_token, api_version=None):
        self.auth_token = auth_token
        self.__refresh_versions()
        if api_version:
            self.set_api_version(api_version)

    # Set the version used for GET requests
    def set_api_version(self, version):
        self.__verify_version_exists(version)
        self.api_version = version
        self.__headers["x-readme-version"] = "v" + version
        if "categories" not in self.get_version():
            self.__refresh_categories()

    # Call the readme API. api_func should be a requests-based function.
    def __api_call(self, api_func, endpoint, print_info, data=None):
        print(print_info + "...   ", end="")
        response = api_func(
            _readme_api_url + endpoint,
            headers=self.__headers,
            auth=HTTPBasicAuth(self.auth_token, ""),
            data=data,
        )
        if response.status_code not in [200, 201, 204]:
            print("Error (code " + str(response.status_code) + "): " + response.text)
            return None
        else:
            print()
            return None if api_func == requests.delete else json.loads(response.text)

    # API GET call.
    # If paginated, gather and concat the output for each page in the endpoint.
    def __api_GET(self, endpoint, print_info=None, paginated=False):
        if not print_info:
            print_info = "API::GET(" + endpoint + ")"
        if paginated:
            i = 1
            out = []
            while True:
                response = self.__api_call(
                    requests.get,
                    endpoint + "?page=" + str(i),
                    print_info + " (page " + str(i) + ")",
                )
                if response is None:
                    return None
                if len(response) is 0:
                    return out
                out += response
                i += 1
        else:
            return self.__api_call(requests.get, endpoint, print_info)

    # API POST call.
    # Data should be passed in as a map. The map will be converted to string.
    def __api_POST(self, endpoint, data, print_info=None):
        if not print_info:
            print_info = "API::POST(" + endpoint + ")"

        # Convert data to str
        data_str = ""
        for x, y in data.items():
            data_str += '"' + x + '":"' + y + '",'
        data_str = ("{" + data_str[:-1] + "}").encode("utf-8")
        data = data_str

        return self.__api_call(requests.post, endpoint, print_info, data)

    # API DELETE call.
    def __api_DELETE(self, endpoint, print_info):
        if not print_info:
            print_info = "API::DELETE(" + endpoint + ")"
        return self.__api_call(requests.delete, endpoint, print_info)

    # Populates version_to_info as a map: "version" -> "version info"
    def __refresh_versions(self):
        response = self.__api_GET("version", print_info="Fetching versions")
        if response:
            self.__versions = {}
            for version in response:
                self.get_versions()[version["version"]] = version

    # Verify a version exists
    def __verify_version_exists(self, version):
        if version not in self.get_versions():
            raise ValueError("Version " + version + " does not exist.")

    # Get all version info
    def get_versions(self):
        return self.__versions

    # Get a version info
    def get_version(self):
        versions = self.get_versions()
        return versions[self.api_version] if self.api_version in versions else None

    # Populates categories as a map: "category title" -> "category ID"
    def __refresh_categories(self):
        version_info = self.get_version()
        version_info["categories"] = {}
        categories = version_info["categories"]
        response = self.__api_GET(
            "categories",
            paginated=True,
            print_info="Fetching categories for version " + self.api_version,
        )
        if response is not None:
            for category in response:
                if category[
                    "reference"
                ]:  # Only get cateories that are in the API reference
                    if category["title"] in categories:
                        print(
                            "Warning: There are two categories with the name "
                            + category["title"]
                            + " for version "
                            + self.api_version
                            + ". Which category this title refers"
                            + " to will be unpredictable."
                        )
                    categories[category["title"]] = category
                    self.__refresh_category_files(category["title"])

    # Populate as a map: map<category, map<title, info object>>
    def __refresh_category_files(self, category):
        self.__verify_category_exists(category)
        category_files = self.__api_GET(
            "categories/" + self.get_category(category)["slug"] + "/docs",
            print_info="Fetching docs in " + category,
        )
        # Populate as a map: map<title, info object>>
        category = self.get_category(category)
        category["files"] = {}
        for file in category_files:
            category["files"][file["title"]] = file

    # Get all category info
    def get_categories(self):
        return self.get_version()["categories"]

    # Get a category info
    def get_category(self, category):
        categories = self.get_categories()
        return categories[category] if category in categories else None

    # Get a categories' file list
    def get_category_files(self, category):
        self.__verify_category_exists(category)
        return self.get_category(category)["files"]

    # Verify a category exists
    def __verify_category_exists(self, category):
        if not self.get_category(category):
            raise ValueError(
                "Category "
                + category
                + " does not exist for version "
                + self.api_version
                + "."
            )

    # Create a version with default settings.
    def create_version(
        self, version, from_version=None, is_stable=False, is_beta=False, is_hidden=True
    ):
        if version in self.get_versions():
            raise ValueError(
                "Version " + version + " already exists! Cannot create it."
            )

        # If no source version, pick the latest one
        if not from_version:
            max_version = 0
            for ver in self.get_versions():
                ver = float(ver)
                if ver > max_version:
                    max_version = ver
            from_version = str(max_version)

        data = {
            "version": "v" + version,
            "is_stable": is_stable,
            "is_beta": is_beta,
            "is_hidden": is_hidden,
            "from": from_version,
        }
        self.get_versions()[version] = self.__api_POST(
            "version", data, "Creating version " + version
        )

    # Update a version
    def update_version(self, version, is_stable=None, is_beta=None, is_hidden=None):
        self.__verify_version_exists(version)
        data = {
            "version": "v" + version,
            "is_stable": is_stable
            if is_stable is not None
            else self.get_versions()[version]["is_stable"],
            "is_beta": is_beta
            if is_beta is not None
            else self.get_versions()[version]["is_beta"],
            "is_hidden": is_hidden
            if is_hidden is not None
            else self.get_versions()[version]["is_hidden"],
        }
        version = self.__api_POST("version", data, "Creating version " + version)
        for k, v in version.items():
            self.get_versions()[version][k] = v

    # Empty a category
    def empty_category(self, category):
        self.__verify_category_exists(category)
        print("Emptying category " + category)
        for title, data in self.get_category_files(category).items():
            self.__api_DELETE(
                "docs/" + data["slug"],
                print_info="    Removing file " + category + "/" + title,
            )
        self.get_category(category)["files"] = {}

    # Delete files in the given category with the given title
    def delete_file_with_title(self, title, category):
        self.__verify_category_exists(category)

        # Search for a file with the same title.
        files = self.get_category_files(category)
        if title in files:
            self.__api_DELETE(
                "docs/" + files[title]["slug"],
                print_info="Removing duplicate file " + category + "/" + title,
            )
            files.pop(title)

    # Uploads all files in the folder at path to ReadMe.
    # Can also upload individual files at path.
    def upload(self, path, category, recursive=False):
        self.__verify_category_exists(category)

        if os.path.isdir(path):
            if recursive:
                # get all subdirs in path and recursively transfer all files in that subdir
                subdirpath = path
                onlydirs = [
                    f
                    for f in os.listdir(subdirpath)
                    if os.path.isdir(os.path.join(subdirpath, f))
                ]
                for dir in onlydirs:
                    self.upload(os.path.join(path, dir), category, recursive)

            # get all filenames in current dir
            files = sorted(
                [
                    os.path.join(path, f)
                    for f in os.listdir(path)
                    if os.path.isfile(os.path.join(path, f))
                ]
            )

            # iterate through all filenames and import the html files
            for currfilename in files:
                self.upload(currfilename, category, recursive)
        elif not os.path.isfile(path):
            raise ValueError("Unable to find file at path: " + path)

        currfilename = path
        if currfilename.find(".html") != -1:
            # open and read file
            file = open(currfilename, "r")
            filecontents = file.read()
            file.close()
            filecontents = filecontents.replace("\\", "&#92;")
            filecontents = filecontents.replace("\n", "\\\\n")
            filecontents = filecontents.replace("¶", "")
            filecontents = filecontents.replace('"', "'")
            filecontents = (
                '[block:html]\\n{\\n \\"html\\": \\"'
                + filecontents
                + '\\"\\n}\\n[/block]'
            )

            firstheadline = os.path.basename(currfilename)[:-5]
            # extract first heading and use as page title
            # soup = BeautifulSoup(filecontents, 'html.parser')
            # for headlines in soup.find_all("h1"):
            # 	firstheadline = headlines.text.strip()
            # 	break

            # Delete files with identical title
            self.delete_file_with_title(firstheadline, category)

            # Set up HTML _reamde_api_url for ReadMe API
            data = {
                "hidden": "false",
                "title": firstheadline,
                "type": "basic",
                "body": filecontents,
                "category": self.get_category(category)["_id"],
            }

            # Create the new page
            out = self.__api_POST(
                "docs", data, "Uploading " + currfilename + " to category " + category
            )
            self.get_category_files(category)[firstheadline] = out
