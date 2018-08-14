# IG02 Data Preparation

In this section, we will show you how to download a publicly available dataset
and load it into an SFrame. This will allow you to try out the Object Detection
toolkit for yourself. To get this dataset into the format expected by our
toolkit, we will rely on many useful SFrame functions.

*Note: Please make sure that you have Turi Create 4.3 or above for these steps*

The dataset that we will use is [INRIA Annotations for Graz-02
(IG02)](https://lear.inrialpes.fr/people/marszalek/data/ig02/)[^1][^2]. Our goal is to
make a detector that can spot bikes and cars. Open a new terminal window and run the following commands:

```
$ mkdir -p ~/Downloads/ig02
$ cd ~/Downloads/ig02
$ curl https://lear.inrialpes.fr/people/marszalek/data/ig02/ig02-v1.0-bikes.zip > bikes.zip
$ curl https://lear.inrialpes.fr/people/marszalek/data/ig02/ig02-v1.0-cars.zip > cars.zip
$ unzip bikes.zip
$ mv readme.txt readme-bikes.txt
$ unzip cars.zip
$ rm bikes.zip cars.zip 
```

Now, you should have the following file structure:

```
~/Downloads/ig02/
    bikes_info.txt
    bikes_test.txt
    bikes_train.txt
    bikes/
        bike_001.image.png
        bike_001.mask.0.png
        bike_001.mask.1.png
        bike_001.mask.2.png
        bike_001.mask.3.png
        bike_002.image.png
        ...
    cars_info.txt
    cars_test.txt
    cars_train.txt
    cars/
        carsgraz_001.image.png
        carsgraz_001.mask.0.png
        carsgraz_002.image.png
        ...
    readme.txt
    readme2.txt
```

This dataset contains masks marking the exact pixels belonging to each object
instance. Each image may have several masks to indicate the presence of
multiple objects. Red pixels indicate object presence and green indicate
presence but with [occlusion](https://en.wikipedia.org/wiki/Occultation). For
object detection, we will convert this richly annotated data to bounding boxes.
Here is a snippet to turn this into an SFrame:

```python
import turicreate as tc
import os

# Change if applicable
ig02_path = '~/Downloads/ig02'

# Load all images in random order
raw_sf = tc.image_analysis.load_images(ig02_path, recursive=True,
                                       random_order=True)

# Split file names so that we can determine what kind of image each row is
# E.g. bike_005.mask.0.png -> ['bike_005', 'mask']
info = raw_sf['path'].apply(lambda path: os.path.basename(path).split('.')[:2])

# Rename columns to 'name' and 'type'
info = info.unpack().rename({'X.0': 'name', 'X.1': 'type'})

# Add to our main SFrame
raw_sf = raw_sf.add_columns(info)

# Extract label (e.g. 'bike') from name (e.g. 'bike_003')
raw_sf['label'] = raw_sf['name'].apply(lambda name: name.split('_')[0])

# Original path no longer needed
del raw_sf['path']

# Split into images and masks
sf_images = raw_sf[raw_sf['type'] == 'image']
sf_masks = raw_sf[raw_sf['type'] == 'mask']

def mask_to_bbox_coordinates(img):
    """
    Takes a tc.Image of a mask and returns a dictionary representing bounding
    box coordinates: e.g. {'x': 100, 'y': 120, 'width': 80, 'height': 120}
    """
    import numpy as np
    mask = img.pixel_data
    if mask.max() == 0:
        return None
    # Take max along both x and y axis, and find first and last non-zero value
    x0, x1 = np.where(mask.max(0))[0][[0, -1]]
    y0, y1 = np.where(mask.max(1))[0][[0, -1]]

    return {'x': (x0 + x1) / 2, 'width': (x1 - x0),
            'y': (y0 + y1) / 2, 'height': (y1 - y0)}

# Convert masks to bounding boxes (drop masks that did not contain bounding box)
sf_masks['coordinates'] = sf_masks['image'].apply(mask_to_bbox_coordinates)

# There can be empty masks (which returns None), so let's get rid of those
sf_masks = sf_masks.dropna('coordinates')

# Combine label and coordinates into a bounding box dictionary
sf_masks = sf_masks.pack_columns(['label', 'coordinates'],
                                 new_column_name='bbox', dtype=dict)

# Combine bounding boxes of the same 'name' into lists
sf_annotations = sf_masks.groupby('name',
                                 {'annotations': tc.aggregate.CONCAT('bbox')})

# Join annotations with the images. Note, some images do not have annotations,
# but we still want to keep them in the dataset. This is why it is important to
# a LEFT join.
sf = sf_images.join(sf_annotations, on='name', how='left')

# The LEFT join fills missing matches with None, so we replace these with empty
# lists instead using fillna.
sf['annotations'] = sf['annotations'].fillna([])

# Remove unnecessary columns
del sf['type']

# Save SFrame
sf.save('ig02.sframe')
```
There are many useful data preparation tips in these steps, so we encourage you
to go through it in an interactive session or a notebook so that you can
inspect the outcome of each command.

After you have run this script, you should have `ig02.sframe` in your current
directory. This SFrame can be used as described in the [Object detection user
guide](README.md).

Alternately, if you don't have access to bitmasks and you're using an image 
annotation tool like the one 
[here](https://github.com/sgp715/simple_image_annotator), here is a code snippet
(thanks to [@Kurry](https://github.com/Kurry)) 
that would convert the CSV you get out of the annotator into an SFrame.

```python
import turicreate as tc
import os

IMAGES_DIR = 'images' # Change if applicable
csv_path = 'out.csv' # assumes CSV column format is image,id,name,xMin,xMax,yMin,yMax
csv_sf = tc.SFrame.read_csv(csv_path)

def row_to_bbox_coordinates(row):
    """
    Takes a row and returns a dictionary representing bounding
    box coordinates: e.g. {'x': 100, 'y': 120, 'width': 80, 'height': 120}
    """
    return {'x': row['xMin'], 'width': (row['xMax'] - row['xMin']),
            'y': row['yMin'], 'height': (row['yMax'] - row['yMin'])}

csv_sf['coordinates'] = csv_sf.apply(row_to_bbox_coordinates)
# delete no longer needed columns
del csv_sf['id'], csv_sf['xMin'], csv_sf['xMax'], csv_sf['yMin'], csv_sf['yMax']
# rename columns
csv_sf = csv_sf.rename({'name': 'label', 'image': 'name'})

# Load all images in random order
sf_images = tc.image_analysis.load_images(images_directory, recursive=True,
    random_order=True)

# Split path to get filename
info = sf_images['path'].apply(lambda path: os.path.basename(path).split('/')[:1])

# Rename columns to 'name'
info = info.unpack().rename({'X.0': 'name'})

# Add to our main SFrame
sf_images = sf_images.add_columns(info)

# Original path no longer needed
del sf_images['path']

# Combine label and coordinates into a bounding box dictionary
csv_sf = csv_sf.pack_columns(['label', 'coordinates'], new_column_name='bbox', dtype=dict)

# Combine bounding boxes of the same 'name' into lists
sf_annotations = csv_sf.groupby('name', 
    {'annotations': tc.aggregate.CONCAT('bbox')})

# Join annotations with the images. Note, some images do not have annotations,
# but we still want to keep them in the dataset. This is why it is important to
# a LEFT join.
sf = sf_images.join(sf_annotations, on='name', how='left')

# The LEFT join fills missing matches with None, so we replace these with empty
# lists instead using fillna.
sf['annotations'] = sf['annotations'].fillna([])

# Save SFrame
sf.save('ig02.sframe')
```

## References

The [IG02](lear.inrialpes.fr/people/marszalek/data/ig02/) dataset is described
further in:

[^1]: Marcin Marszałek and Cordelia Schmid. [Accurate Object Localization with Shape Masks.](http://lear.inrialpes.fr/pubs/2007/MS07a/) IEEE Conference on Computer Vision & Pattern Recognition, 2007.

[^2]: A. Opelt, A. Pinz, M. Fussenegger, P. Auer. [Generic Object Recognition with Boosting.](http://www.emt.tugraz.at/~opelt/download_data/Generic_object_recognition_with_boosting.pdf) IEEE Transactions on Pattern Recognition and Machine Intelligence (PAMI), Vol.28, No. 3, March 2006.

