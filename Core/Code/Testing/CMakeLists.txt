MITK_CREATE_MODULE_TESTS(LABELS MITK-Core)
# MITK_INSTALL_TARGETS(EXECUTABLES MitkTestDriver)

mitkAddCustomModuleTest(mitkDICOMLocaleTest_spacingOk_CT mitkDICOMLocaleTest ${MITK_DATA_DIR}/spacing-ok-ct.dcm)
mitkAddCustomModuleTest(mitkDICOMLocaleTest_spacingOk_MR mitkDICOMLocaleTest ${MITK_DATA_DIR}/spacing-ok-mr.dcm)
mitkAddCustomModuleTest(mitkDICOMLocaleTest_spacingOk_SC mitkDICOMLocaleTest ${MITK_DATA_DIR}/spacing-ok-sc.dcm)

mitkAddCustomModuleTest(mitkVolumeCalculatorTest_Png2D-bw mitkVolumeCalculatorTest ${MITK_DATA_DIR}/Png2D-bw.png ${MITK_DATA_DIR}/Pic2DplusT.nrrd)

mitkAddCustomModuleTest(mitkEventMapperTest_Test1And2 mitkEventMapperTest ${MITK_DATA_DIR}/TestStateMachine1.xml ${MITK_DATA_DIR}/TestStateMachine2.xml)

#mitkAddCustomModuleTest(mitkNodeDependentPointSetInteractorTest mitkNodeDependentPointSetInteractorTest ${MITK_DATA_DIR}/Pic3D.pic.gz ${MITK_DATA_DIR}/BallBinary30x30x30.pic.gz)
mitkAddCustomModuleTest(mitkNodeDependentPointSetInteractorTest mitkNodeDependentPointSetInteractorTest  ${MITK_DATA_DIR}/Pic3D.nrrd ${MITK_DATA_DIR}/BallBinary30x30x30.nrrd)

mitkAddCustomModuleTest(mitkDataStorageTest_US4DCyl mitkDataStorageTest ${MITK_DATA_DIR}/US4DCyl.nrrd)

mitkAddCustomModuleTest(mitkStateMachineFactoryTest_TestStateMachine1_2 mitkStateMachineFactoryTest ${MITK_DATA_DIR}/TestStateMachine1.xml ${MITK_DATA_DIR}/TestStateMachine2.xml)

mitkAddCustomModuleTest(mitkDicomSeriesReaderTest_CTImage mitkDicomSeriesReaderTest ${MITK_DATA_DIR}/TinyCTAbdomen)

mitkAddCustomModuleTest(mitkPointSetReaderTest mitkPointSetReaderTest ${MITK_DATA_DIR}/PointSetReaderTestData.mps)

mitkAddCustomModuleTest(mitkImageTest_4DImageData mitkImageTest ${MITK_DATA_DIR}/US4DCyl.nrrd)
mitkAddCustomModuleTest(mitkImageTest_2D+tImageData mitkImageTest ${MITK_DATA_DIR}/Pic2DplusT.nrrd)
mitkAddCustomModuleTest(mitkImageTest_3DImageData mitkImageTest ${MITK_DATA_DIR}/Pic3D.nrrd)
mitkAddCustomModuleTest(mitkImageTest_brainImage mitkImageTest ${MITK_DATA_DIR}/brain.mhd)
#mitkAddCustomModuleTest(mitkImageTest_color2DImage mitkImageTest ${MITK_DATA_DIR}/NrrdWritingTestImage.jpg)

mitkAddCustomModuleTest(mitkIOUtilTest mitkIOUtilTest #test for a randomly chosen Pic3D swivelled slice
                        ${MITK_DATA_DIR}/Pic3D.nrrd
                        ${MITK_DATA_DIR}/pointSet.mps
                        ${MITK_DATA_DIR}/binary.stl
)


mitkAddCustomModuleTest(mitkLevelWindowManagerTest mitkLevelWindowManagerTest
                        ${MITK_DATA_DIR}/Pic3D.nrrd
)


if(WIN32 OR APPLE OR MITK_ENABLE_GUI_TESTING) ### since the rendering test's do not run in ubuntu, yet, we build them only for other systems or if the user explicitly sets the variable MITK_ENABLE_GUI_TESTING
mitkAddCustomModuleTest(mitkImageVtkMapper2D_rgbaImage640x480 mitkImageVtkMapper2DTest
                        ${MITK_DATA_DIR}/RenderingTestData/rgbaImage.png #input image to load in data storage
                        -V ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/rgbaImage640x480REF.png #corresponding reference screenshot
)
mitkAddCustomModuleTest(mitkImageVtkMapper2D_pic3d640x480 mitkImageVtkMapper2DTest #test for standard Pic3D axial slice
                        ${MITK_DATA_DIR}/Pic3D.nrrd #input image to load in data storage
                        -V ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/pic3d640x480REF.png #corresponding reference screenshot
)
mitkAddCustomModuleTest(mitkImageVtkMapper2D_pic3dColorBlue640x480 mitkImageVtkMapper2DColorTest #test for color property (=blue) Pic3D sagittal slice
                        ${MITK_DATA_DIR}/Pic3D.nrrd #input image to load in data storage
                        -V ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/pic3dColorBlue640x480REF.png #corresponding reference screenshot
)
mitkAddCustomModuleTest(mitkImageVtkMapper2D_pic3dLevelWindow640x480 mitkImageVtkMapper2DLevelWindowTest #test for levelwindow property (=blood) #Pic3D sagittal slice
                        ${MITK_DATA_DIR}/Pic3D.nrrd #input image to load in data storage
                        -V ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/pic3dLevelWindowBlood640x480REF.png #corresponding reference #screenshot
)
#mitkAddCustomModuleTest(mitkImageVtkMapper2D_pic3dOpacity640x480 mitkImageVtkMapper2DOpacityTest #test for opacity (=0.5) Pic3D coronal slice
#                        ${MITK_DATA_DIR}/Pic3D.nrrd #input image to load in data storage
#                        -V ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/pic3dOpacity640x480REF.png #corresponding reference screenshot
#)
mitkAddCustomModuleTest(mitkImageVtkMapper2D_pic3dSwivel640x480 mitkImageVtkMapper2DSwivelTest #test for a randomly chosen Pic3D swivelled slice
                        ${MITK_DATA_DIR}/Pic3D.nrrd #input image to load in data storage
                        -V ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/pic3dSwivel640x480REF.png #corresponding reference screenshot
)

mitkAddCustomModuleTest(mitkSurfaceVtkMapper3DTest_TextureProperty mitkSurfaceVtkMapper3DTest
                        ${MITK_DATA_DIR}/ToF-Data/Kinect_LiverPhantom.vtp
                        ${MITK_DATA_DIR}/ToF-Data/Kinect_LiverPhantom_RGBImage.nrrd
                        -V
                        ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/texturedLiver640x480REF.png #corresponding reference screenshot
)

mitkAddCustomModuleTest(mitkImageVtkMapper2DTransferFunctionTest_Png2D-bw mitkImageVtkMapper2DTransferFunctionTest
                        ${MITK_DATA_DIR}/Png2D-bw.png
                        -V ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/Png2D-bw-TransferFunctionRGBImage640x480REF.png #corresponding reference screenshot
)

#mitkAddCustomModuleTest(mitkSurfaceVtkMapper3DTexturedSphereTest_Football mitkSurfaceVtkMapper3DTexturedSphereTest
#                        ${MITK_DATA_DIR}/RenderingTestData/texture.jpg #input texture
#                        -V
#                        ${MITK_DATA_DIR}/RenderingTestData/ReferenceScreenshots/texturedSphere640x480REF.png corresponding reference screenshot
#)

SET_PROPERTY(TEST mitkImageVtkMapper2D_rgbaImage640x480 mitkImageVtkMapper2D_pic3d640x480 mitkImageVtkMapper2D_pic3dColorBlue640x480 mitkImageVtkMapper2D_pic3dLevelWindow640x480 mitkImageVtkMapper2D_pic3dSwivel640x480 mitkImageVtkMapper2DTransferFunctionTest_Png2D-bw #mitkSurfaceVtkMapper3DTest_TextureProperty #mitkSurfaceVtkMapper3DTexturedSphereTest_Football #mitkImageVtkMapper2D_pic3dOpacity640x480
PROPERTY RUN_SERIAL TRUE)

endif()

add_test(mitkPointSetLocaleTest  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTDRIVER} mitkPointSetLocaleTest ${MITK_DATA_DIR}/pointSet.mps)
set_property(TEST mitkPointSetLocaleTest PROPERTY LABELS MITK-Core)

add_test(mitkImageWriterTest_nrrdImage ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTDRIVER} mitkImageWriterTest ${MITK_DATA_DIR}/NrrdWritingTestImage.jpg)
add_test(mitkImageWriterTest_2DPNGImage ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTDRIVER} mitkImageWriterTest ${MITK_DATA_DIR}/Png2D-bw.png)
add_test(mitkImageWriterTest_rgbPNGImage ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTDRIVER} mitkImageWriterTest ${MITK_DATA_DIR}/RenderingTestData/rgbImage.png)
add_test(mitkImageWriterTest_rgbaPNGImage ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTDRIVER} mitkImageWriterTest ${MITK_DATA_DIR}/RenderingTestData/rgbaImage.png)
set_property(TEST mitkImageWriterTest_nrrdImage PROPERTY LABELS MITK-Core)
set_property(TEST mitkImageWriterTest_2DPNGImage PROPERTY LABELS MITK-Core)
set_property(TEST mitkImageWriterTest_rgbPNGImage PROPERTY LABELS MITK-Core)
set_property(TEST mitkImageWriterTest_rgbaPNGImage PROPERTY LABELS MITK-Core)

add_subdirectory(DICOMTesting)

