import urllib.request, urllib.parse, urllib.error

def _raw_urlretrieve(url, target_file, context=None):
    handle = urllib.request.urlopen(url)#, context=context)
    if handle.getcode() >= 300:
        raise IOError("HTTP Error " + str(handle.getcode()))
    with open(target_file, 'wb') as output:
        while True:
            data = handle.read(1024*1024)
            if data:
                output.write(data)
            else:
                break


def urlretrieve(url, target_file):
    print("Downloading " + url + " to " + target_file)
    _raw_urlretrieve(url, target_file)
