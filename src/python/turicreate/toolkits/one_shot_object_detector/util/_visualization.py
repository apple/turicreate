from turicreate.toolkits.object_detector.util._visualization import (
    draw_bounding_boxes as _draw_bounding_boxes,
)


def draw_bounding_boxes(images, annotations, confidence_threshold=0):
    """
    Visualizes bounding boxes (ground truth or predictions) by
    returning annotated copies of the images.

    Parameters
    ----------
    images: SArray or Image
        An `SArray` of type `Image`. A single `Image` instance may also be
        given.

    annotations: SArray or list
        An `SArray` of annotations (either output from the
        `ObjectDetector.predict` function or ground truth). A single list of
        annotations may also be given, provided that it is coupled with a
        single image.

    confidence_threshold: float
        Confidence threshold can limit the number of boxes to draw. By
        default, this is set to 0, since the prediction may have already pruned
        with an appropriate confidence threshold.

    Returns
    -------
    annotated_images: SArray or Image
        Similar to the input `images`, except the images are decorated with
        boxes to visualize the object instances.

    See Also
    --------
    predict

    Examples
    --------
    .. sourcecode:: python

        # Make predictions
        >>> pred = model.predict(data)
        >>> predictions_with_bounding_boxes = tc.one_shot_object_detector.util.draw_bounding_boxes(data['images'], pred)
        >>> predictions_with_bounding_boxes.explore()
    """
    return _draw_bounding_boxes(images, annotations, confidence_threshold)
