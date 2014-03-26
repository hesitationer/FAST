#include "SmartPointers.hpp"
#include "Exception.hpp"
#include "ImageImporter2D.hpp"
#include "ImageExporter2D.hpp"
#include "VTKImageExporter.hpp"
#include "VTKImageImporter.hpp"
#include "ITKImageExporter.hpp"
#include "ImageStreamer2D.hpp"
#include "DeviceManager.hpp"
#include "GaussianSmoothingFilter2D.hpp"

#include <vtkVersion.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkImageMapper.h>
#include <vtkActor2D.h>

using namespace fast;

Image2D::pointer create() {
    // Example of importing one 2D image
    ImageImporter2D::pointer importer = ImageImporter2D::New();
    importer->setFilename("lena.jpg");
    return importer->getOutput();
}

int main(int argc, char ** argv) {

    // Get a GPU device and set it as the default device
    DeviceManager& deviceManager = DeviceManager::getInstance();
    deviceManager.setDefaultDevice(deviceManager.getOneGPUDevice());




    // Example of importing, processing and exporting a 2D image
    ImageImporter2D::pointer importer = ImageImporter2D::New();
    importer->setFilename("lena.jpg");
    GaussianSmoothingFilter2D::pointer filter = GaussianSmoothingFilter2D::New();
    filter->setInput(importer->getOutput());
    filter->setMaskSize(7);
    filter->setStandardDeviation(10);
    Image2D::pointer filteredImage = filter->getOutput();
    ImageExporter2D::pointer exporter = ImageExporter2D::New();
    exporter->setFilename("test.jpg");
    exporter->setInput(filteredImage);
    exporter->update();





    // Example of creating a pipeline in another scope and updating afterwards
    Image2D::pointer image2 = create();
    std::cout << "after create" << std::endl;
    image2->update();





    // Example of streaming 2D images
    ImageStreamer2D::pointer streamer = ImageStreamer2D::New();
    streamer->setFilenameFormat("test_#.jpg");
    Image2Dt::pointer dynamicImage = streamer->getOutput();
    dynamicImage->update();





    // VTK Export and render example
    vtkSmartPointer<VTKImageExporter> vtkExporter = VTKImageExporter::New();
    vtkExporter->SetInput(filteredImage);
    vtkSmartPointer<vtkImageData> vtkImage = vtkExporter->GetOutput();
    vtkExporter->Update();





    // VTK mess for getting the image on screen
    vtkSmartPointer<vtkImageMapper> imageMapper = vtkSmartPointer<vtkImageMapper>::New();
#if VTK_MAJOR_VERSION <= 5
    imageMapper->SetInputConnection(vtkImage->GetProducerPort());
#else
    imageMapper->SetInputData(vtkImage);
#endif
    imageMapper->SetColorWindow(1);
    imageMapper->SetColorLevel(0.5);

    vtkSmartPointer<vtkActor2D> imageActor = vtkSmartPointer<vtkActor2D>::New();
    imageActor->SetMapper(imageMapper);

    // Setup renderers and render window
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(filteredImage->getWidth(), filteredImage->getHeight());

    // Setup render window interactor
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

    vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
    renderWindowInteractor->SetInteractorStyle(style);
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderer->AddActor2D(imageActor);
    renderWindow->Render();
    renderWindowInteractor->Start();





    // ITK Export example
    typedef itk::Image<float, 2> ImageType;
    ITKImageExporter<ImageType>::Pointer itkExporter = ITKImageExporter<ImageType>::New();
    itkExporter->SetInput(filteredImage);
    ImageType::Pointer itkImage = itkExporter->GetOutput();
    itkExporter->Update();




    // VTK Import example
    VTKImageImporter::pointer vtkImporter = VTKImageImporter::New();
    vtkImporter->setInput(vtkImage);
    Image2D::pointer importedImage = vtkImporter->getOutput();
    vtkImporter->update();
}
