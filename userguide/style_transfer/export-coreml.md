# Deployment to Core ML

Style trasnfer models created in Turi Create can easily be deployed to
Core ML into an iOS or macOS application.

#### Export to Core ML {#coreml}

Exporting the trained style transfer model is done using:
```python
model.export_coreml('MyStyleTransfer.mlmodel')
```

This Core ML model takes an input image and index of type MultiArray and
outputs a stylizedImage.

Drag and drop MyStyleTransfer.mlmodel into your Xcode project and add it
to your app by ticking the appropriate Target Membership check box.  An
arrow next to MyCustomStyleTransfer should appear:

![Xcode view of MyStyleTransfer.mlmodel](images/xcode_style_transfer.png)

If your style transfer model has 8 styles and you want to stlyize using
the 4th style, then index should be

```swift
let numStyles  = 8
let styleIndex = 3

let styleArray = try? MLMultiArray(shape: [numStyles] as [NSNumber] dataType: MLMultiArrayDataType.double)

for i in 0...((styleArray?.count)!-1) {
    styleArray?[i] = 0.0
}
styleArray?[styleIndex] = 1.0
```

Now, you can stylize your images using:
```swift
let mlModel = MyStyleTransferModel()
let visionModel = try VNCoreMLModel(for: mlModel)

let styleTransfer = VNCoreMLRequest(model: visionModel, completionHandler: { (request, error) in
        guard let results = request.results else { return }

    for case let styleTransferedImage as VNPixelBufferObservation in results {
        let imageLayer = CALayer()
        imageLayer.contents = CIImage(cvPixelBuffer: styleTransferedImage.pixelBuffer, options: [:])
    }
})
```