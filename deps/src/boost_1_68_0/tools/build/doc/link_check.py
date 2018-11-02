import os
import glob
import re
import pprint

def main():
    links = {}
    ids = {}
    files = []
    files.extend(glob.glob(os.path.join(os.getcwd(), 'src', '*.adoc')))
    # files.extend(glob.glob(os.path.join(os.getcwd(), '..', 'src', 'tools', '*.jam')))
    for adoc_file in sorted(files):
        with file(adoc_file) as adoc:
            text = adoc.read()
            #print os.path.basename(adoc_file)
            for link in re.findall('link:#[^\[]+', text):
                link_ref = link.split('#')[1]
                link_loc = links.get(link_ref,[])
                link_loc.append(os.path.basename(adoc_file))
                links[link_ref] = link_loc
            for link in re.findall('<<[^>]+', text):
                link_ref = link[2:]
                link_loc = links.get(link_ref,[])
                link_loc.append(os.path.basename(adoc_file))
                links[link_ref] = link_loc
            for id in re.findall('[\[][\[\#][^\]]+', text):
                id_ref = id[2:]
                id_loc = ids.get(id_ref,[])
                id_loc.append(os.path.basename(adoc_file))
                ids[id_ref] = id_loc
            for id in re.findall('anchor:[^\[]+', text):
                id_ref = id[7:]
                id_loc = ids.get(id_ref,[])
                id_loc.append(os.path.basename(adoc_file))
                ids[id_ref] = id_loc
    #pprint.pprint(links)
    pprint.pprint(ids)
    for link in sorted(links.keys()):
        if not link in ids:
            print "ERROR: missing ref for id ",link," in files: ",", ".join(links[link])

if __name__ == "__main__":
    main()
