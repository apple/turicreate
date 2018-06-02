# Deployment to Core ML

This page describes how to deploy your style transfer model to an iOS or macOS app using Core ML.

#### Export to Core ML {#coreml}

Exporting the trained style transfer model into Core ML is as simple as
```python
model.export_coreml('MyStyleTransfer.mlmodel')
```
This Core ML model takes an input image and index of type MultiArray and outputs a stylizedImage.

Drag and drop MyStyleTransfer.mlmodel into your Xcode project and add it to your app by ticking the appropriate Target Membership check box. 
An arrow next to MyStyleTransfer should appear:

![Xcode view of MyStyleTransfer.mlmodel](images/xcode_style_transfer.png)

If your style transfer model has 8 styles and you want to stlyize using the 4th style, then index should be
```swift
let num_styles = 3
let style_idx = 3
let array = try? MLMultiArray(shape: [num_styles] as [NSNumber], dataType: MLMultiArrayDataType.double)
    for i in 0...((array?.count)!-1) {
        if(i == style_idx){
            array![i] = NSNumber(value: 1.0)
        }else{
            array![i] = NSNumber(value: 0.0)
        }
    }
```

Then you can pass image and array to the model as
```swift
    import Cocoa
    import Vision
    import CoreML
    
    
    let mlModel = MyStyleTransferModel()
    let visionModel = try VNCoreMLModel(for: mlModel)
    
    let styleTranser = VNCoreMLRequest(model: visionModel, completionHandler: { (request, error) in
        guard let results = request.results else { return }

    for case let styleTransferedImage as VNPixelBufferObservation in results {
        let imageLayer = CALayer()
        imageLayer.contents = CIImage(cvPixelBuffer: styleTransferedImage.pixelBuffer, options: [:])
    }
})

```
