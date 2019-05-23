import turicreate.toolkits._internal_utils as _tkutl

def check_one_shot_input(data, target):
    if not isinstance(target, str):
        raise TypeError("'target' must be of type string")
    if isinstance(dataset, _tc.SFrame):
        image_column_name = _tkutl._find_only_image_column(dataset)
        target_column_name = target
        dataset_to_augment = dataset
    elif isinstance(dataset, _tc.Image):
        image_column_name = "image"
        target_column_name = "target"
        dataset_to_augment = _tc.SFrame({image_column_name: dataset,
                                         target_column_name: target})
    else:
        raise TypeError("'data' must be of type SFrame or Image")
    return dataset_to_augment, image_column_name, target_column_name
