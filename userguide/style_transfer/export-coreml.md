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

let imageSize = CGSize(width: 600, height: 600)

func stylize(img:UIImage, index:Int, image_size: CGSize) -> UIImage {
	let image = resizeImage(image: img, targetSize: image_size)
    let ciImage = CIImage(image: image)
    let context = CIContext(options: nil)
    let cgImage = context.createCGImage(ciImage!, from: ciImage!.extent)
    var pxbuffer: CVPixelBuffer? = nil
    let options: NSDictionary = [:]
    let bytesPerRow = cgImage!.bytesPerRow
    let dataFromImageDataProvider = cgImage?.dataProvider!.data
        
    CVPixelBufferCreateWithBytes(
        kCFAllocatorDefault,
        image_size.width,
        image_size.height,
        kCVPixelFormatType_32BGRA,
        CFDataGetMutableBytePtr(dataFromImageDataProvider as! CFMutableData),
        bytesPerRow,
        nil,
        nil,
        options,
        &pxbuffer)

    var pred_img:MyStyleTransferModelOutput;
    do{
        pred_img = try self.model.prediction(image: pxbuffer!, index: array!)
        var ml_multi_array = (pred_img.stylizedImage)
        var unwrapped_ml = ml_multi_array
        var uiImage = getUIImage(img: unwrapped_ml)
        return uiImage
    } catch {
        print("Unexpected Prediction Error: \(error)")
    }
}

let prediction_image = stylize(img: InputImage, index: styleArray, image_size: CGSize(600, 600))
```