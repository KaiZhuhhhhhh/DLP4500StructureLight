/** @file       3D_Scanner_GrayCode_LCr4500_PG_FlyCap2_C.cpp
 *  @brief      TIDA-00254 3D Scanner Application using DLP LightCrafter 4500 and Point Grey Flea3 USB camera
 *  @copyright  2016 Texas Instruments Incorporated - http://www.ti.com/ ALL RIGHTS RESERVED
 */
#include<winsock2.h>


#include <Windows.h>
#include <stdlib.h>
#include <string>       // Included for std::string
#include <thread>       // Included for std::thread
#include <functional>   // Included for std::ref
#include <dlp_sdk.hpp>  // Included for DPL Structured Light SDK
//#include "dlp_platforms/lightcrafter_4500/dlpc350_api.hpp"
//#include <fstream>
#include <iostream>
//using namespace std;


void GenerateCameraCalibrationBoard(dlp::Camera       *camera,
                                    const std::string &camera_calib_settings_file,
                                    const std::string &camera_calib_board_name){
    dlp::ReturnCode ret;

    dlp::CmdLine::Print();
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("<<<<<<<<<<<<<<<<<<<<<< Generate Camera Calibration Board >>>>>>>>>>>>>>>>>>>>>>");

    // Camera Calibration Variables
    dlp::Calibration::Camera    camera_calib;
    dlp::Parameters             camera_calib_settings;
    dlp::Image                  camera_calib_board_image;

    // Check that camera is NOT null
    if(!camera) return;

    // Load calibration settings
    dlp::CmdLine::Print("Loading camera calibration settings...");
    if(camera_calib_settings.Load(camera_calib_settings_file).hasErrors()){

        // Camera calibration settings file did NOT load
        dlp::CmdLine::Print("Could NOT load camera calibration settings!");
        dlp::CmdLine::Print(ret.ToString());
        return;
    }

    // Setup camera calibration for current camera
    camera_calib.SetCamera((*camera));

    dlp::CmdLine::Print("Setting up camera calibration...");
    ret = camera_calib.Setup(camera_calib_settings);
    if(ret.hasErrors()){
        dlp::CmdLine::Print("Camera calibration setup FAILED!");
        dlp::CmdLine::Print(ret.ToString());
        return;
    }

    // Genererate and save the calibration image
    dlp::CmdLine::Print("Generating camera calibration board...");
    camera_calib.GenerateCalibrationBoard(&camera_calib_board_image);

    // Save the calibration board
    dlp::CmdLine::Print("Saving calibration board...");
    camera_calib_board_image.Save(camera_calib_board_name);

    // Wait for user to print calibration board
    double side_length;

    dlp::CmdLine::Print("\nPlease print the camera calibration board and attach it to a flat surface");
    dlp::CmdLine::Print("The calibration board image can be found in: " + camera_calib_board_name);
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("Once the calibration image has been printed and attached ");
    dlp::CmdLine::Print("to a flat surface, measure the size of the square on the board");
    dlp::CmdLine::Print("\nNOTE: Enter the measurement in the units desired for the ");
    dlp::CmdLine::Print("      point cloud (i.e. mm, in, cm, etc.)\n");
    dlp::CmdLine::Print("\nNOTE: Both camera and system calibrations must be redone!");

    // Retrieve the measurements
    dlp::CmdLine::Print();
    dlp::CmdLine::Get(side_length,"Enter the length of the square (do NOT include units): ");
    dlp::CmdLine::Print();

    dlp::CmdLine::Print("Saving feature distances to calibration settings file...");
    dlp::CmdLine::Print();
    camera_calib_settings.Set(dlp::Calibration::Parameters::BoardFeatureRowDistance(side_length));
    camera_calib_settings.Set(dlp::Calibration::Parameters::BoardFeatureColumnDistance(side_length));
    camera_calib_settings.Save(camera_calib_settings_file);
}


void PrepareProjectorPatterns( dlp::DLP_Platform    *projector,
                               const std::string    &projector_calib_settings_file,
                               dlp::StructuredLight *structured_light_vertical,
                               const std::string    &structured_light_vertical_settings_file,
                               dlp::StructuredLight *structured_light_horizontal,
                               const std::string    &structured_light_horizontal_settings_file,
                               const bool            previously_prepared,
                               unsigned int         *total_pattern_count){

    dlp::ReturnCode ret;

    dlp::CmdLine::Print();
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("<<<<<<<<<<<<<<<<<<<<<< Load DLP LightCrafter 4500 Firmware >>>>>>>>>>>>>>>>>>>>>>");

    // Projector Calibration Variables
    dlp::Calibration::Projector projector_calib;
    dlp::Parameters             projector_calib_settings;

    // Structured Light Sequence
    dlp::Pattern::Sequence calibration_patterns;
    dlp::Pattern::Sequence vertical_patterns;
    dlp::Pattern::Sequence horizontal_patterns;
    dlp::Pattern::Sequence all_patterns;

    // Structured Light Settings
    dlp::Parameters         structured_light_vertical_settings;
    dlp::Parameters         structured_light_horizontal_settings;

    projector_calib.SetDebugEnable(false);


    // Check that projector is NOT null
    if(!projector) return;

    // Check if projector is connected
    if(!projector->isConnected()){
        dlp::CmdLine::Print("Projector NOT connected!");
        return;
    }

    // Load projector calibration settings
    dlp::CmdLine::Print("Loading projector calibration settings...");
    if(projector_calib_settings.Load(projector_calib_settings_file).hasErrors()){

        // Projector calibration settings file did NOT load
        dlp::CmdLine::Print("Could NOT load projector calibration settings!");
        dlp::CmdLine::Print(ret.ToString());
        return;
    }


    // Setup projector calibration for current projector
    projector_calib.SetDlpPlatform((*projector));

    // Add temporary camera resoltution settings
    projector_calib_settings.Set(dlp::Calibration::Parameters::ImageColumns(1000));
    projector_calib_settings.Set(dlp::Calibration::Parameters::ImageRows(1000));


    dlp::CmdLine::Print("Setting up projector calibration...");
    ret = projector_calib.Setup(projector_calib_settings);
    if(ret.hasErrors()){
        dlp::CmdLine::Print("Projector calibration setup FAILED: ");
        dlp::CmdLine::Print(ret.ToString());
        return;
    }

    // Create the calibration image and convert to monochrome
    dlp::CmdLine::Print("Generating projector calibration board...");
    dlp::Image   projector_calibration_board_image;
    projector_calib.GenerateCalibrationBoard(&projector_calibration_board_image);
    projector_calibration_board_image.ConvertToMonochrome();

    // Create calibration pattern
    dlp::CmdLine::Print("Generating projector calibration pattern...");
    dlp::Pattern calib_pattern;
    calib_pattern.bitdepth  = dlp::Pattern::Bitdepth::MONO_1BPP;
    calib_pattern.color     = dlp::Pattern::Color::WHITE;
    calib_pattern.data_type = dlp::Pattern::DataType::IMAGE_DATA;
    calib_pattern.image_data.Create(projector_calibration_board_image);

    // Add the calibration pattern to the calibration pattern sequence
    calibration_patterns.Add(calib_pattern);

    // Setup the strucutred light patterns
    if(!structured_light_vertical)   return;
    if(!structured_light_horizontal) return;

    // Load the vertical structured light settings
    dlp::CmdLine::Print("Loading vertical structured light settings...");
    if(structured_light_vertical_settings.Load(structured_light_vertical_settings_file).hasErrors()){
        // Structured light settings file did NOT load
        dlp::CmdLine::Print("Structured light settings did NOT load successfully");
        return;
    }

    // Load the horizontal structured light settings
    dlp::CmdLine::Print("Loading horizontal structured light settings...");
    if(structured_light_horizontal_settings.Load(structured_light_horizontal_settings_file).hasErrors()){
        // Structured light settings file did NOT load
        dlp::CmdLine::Print("Structured light settings did NOT load successfully");
        return;
    }

    structured_light_horizontal->SetDlpPlatform((*projector));
    structured_light_vertical->SetDlpPlatform((*projector));

    dlp::CmdLine::Print("Setting up vertical structured light module...");
    dlp::CmdLine::Print(structured_light_vertical->Setup(structured_light_vertical_settings).ToString());

    dlp::CmdLine::Print("Setting up horizontal structured light module...");
    dlp::CmdLine::Print(structured_light_horizontal->Setup(structured_light_horizontal_settings).ToString());

    // Generate the pattern sequence
    dlp::CmdLine::Print("Generating vertical structured light module patterns...");
    structured_light_vertical->GeneratePatternSequence(&vertical_patterns);

    dlp::CmdLine::Print("Generating horizontal structured light module patterns...");
    structured_light_horizontal->GeneratePatternSequence(&horizontal_patterns);

    // Combine all of the pattern sequences
    dlp::CmdLine::Print("Combining all patterns for projector...");
    all_patterns.Add(calibration_patterns);
    all_patterns.Add(vertical_patterns);
    all_patterns.Add(horizontal_patterns);


    // If previously_prepared is false, the module should assume that the projector has NOT been prepared previously
    // This is important for the LightCrafter 4500 since the images are stored in the firmware
    // Therefore firmware must be uploaded to prepare the projector, but this only needs to occur once
    dlp::Parameters force_preparation;
    force_preparation.Set(dlp::DLP_Platform::Parameters::SequencePrepared(previously_prepared));
    projector->Setup(force_preparation);

    // If the firmware needs to be uploaded, display debug messages so progress can be observed
    projector->SetDebugEnable(!previously_prepared);


    // Prepare projector
    dlp::CmdLine::Print("Preparing projector for calibration and structured light...");
    ret = projector->PreparePatternSequence(all_patterns);
    projector->SetDebugEnable(false);

    if( ret.hasErrors()){
        dlp::CmdLine::Print("Projector prepare sequence FAILED: ");
        dlp::CmdLine::Print(ret.ToString());
        (*total_pattern_count) = 0;
    }

    dlp::CmdLine::Print("Projector prepared");
    (*total_pattern_count) = all_patterns.GetCount();
}

void CalibrateCamera(dlp::Camera       *camera,
                     const std::string &camera_calib_settings_file,
                     const std::string &camera_calib_data_file,
                     const std::string &camera_calib_image_file_names,
                     dlp::DLP_Platform *projector){
    dlp::ReturnCode ret;

    dlp::CmdLine::Print();
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("<<<<<<<<<<<<<<<<<<<<<< Calibrating the Camera >>>>>>>>>>>>>>>>>>>>>>");

    // Camera Calibration Variables
    dlp::Calibration::Camera    camera_calib;
    dlp::Calibration::Data      camera_calib_data;
    dlp::Parameters             camera_calib_settings;

    // Check that camera is NOT null
    if(!camera) return;

    // Check if camera is connected
    if(!camera->isConnected()){
        dlp::CmdLine::Print("Camera NOT connected!");
        return;
    }

    // Check that projector is NOT null
    if(!projector) return;

    // Check if projector is connected
    if(!projector->isConnected()){
        dlp::CmdLine::Print("Projector NOT connected!");
        return;
    }

    // Load calibration settings
    dlp::CmdLine::Print("Loading camera calibration settings...");
    if(camera_calib_settings.Load(camera_calib_settings_file).hasErrors()){

        // Camera calibration settings file did NOT load
        dlp::CmdLine::Print("Could NOT load camera calibration settings!");
        dlp::CmdLine::Print(ret.ToString());

        return;
    }

    // Setup camera calibration for current camera
    camera_calib.SetCamera((*camera));

    dlp::CmdLine::Print("Setting up camera calibration");
    ret = camera_calib.Setup(camera_calib_settings);
    if(ret.hasErrors()){
        dlp::CmdLine::Print("Camera calibration setup FAILED: ");
        dlp::CmdLine::Print(ret.ToString());
        return;
    }

    // Project a white pattern to illuminate calibration board
    dlp::CmdLine::Print("Projecting white pattern to illuminate calibration board");
    projector->ProjectSolidWhitePattern();


    // Get the number of calibration images needed
    unsigned int boards_success;
    unsigned int boards_required;
    camera_calib.GetCalibrationProgress(&boards_success,&boards_required);

    dlp::CmdLine::Print("Calibrating camera...");
    dlp::CmdLine::Print("Calibration boards required = ", boards_required);
    dlp::CmdLine::Print("Calibration boards captured = ", boards_success);


    // Display the calibration instructions
    dlp::CmdLine::Print("\nRead the following instructions in their entirety!");
    dlp::CmdLine::PressEnterToContinue("Press ENTER when ready to read the instructions...");
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("Camera calibration instructions:");
    dlp::CmdLine::Print("1. After the live view has started, place the calibration board so it is");
    dlp::CmdLine::Print("   completely visible within the camera frame and is stationary");
    dlp::CmdLine::Print("   - The board should be placed at the desired scanning distance");
    dlp::CmdLine::Print("2. Adjust the camera's aperture and focus until the image is well");
    dlp::CmdLine::Print("   illuminated and sharp");
    dlp::CmdLine::Print("   - Lock the aperture and focus in place if possible");
    dlp::CmdLine::Print("   - If the camera focus or aperture is adjusted, the calibration must be redone");
    dlp::CmdLine::Print("3. Select the live view window and press the SPACE_BAR key to capture");
    dlp::CmdLine::Print("   the calibration board's current position");
    dlp::CmdLine::Print("4. Move the calibration board and repeat step 3 until calibration is complete");
    dlp::CmdLine::Print("   - Capture images with the board at different distances from the camera");
    dlp::CmdLine::Print("   - Capture images with the board at different positions in the camera frame");
    dlp::CmdLine::Print("   - The position of the camera can be changed as well rather than moving the");
    dlp::CmdLine::Print("     board");
    dlp::CmdLine::Print("\nNOTE: Press ESC key to quit the calibration routine");
    dlp::CmdLine::Print();
    dlp::CmdLine::PressEnterToContinue("Press ENTER after reading the above instructions...");

    // Start camera if it hasn't been started
    if(!camera->isStarted()){
        if(camera->Start().hasErrors()){
            dlp::CmdLine::Print("Could NOT start camera! \n");
            return;
        }
        else{
            dlp::CmdLine::Print("Started camera. \n");
        }
    }

    // Give the camera some time to fill the image buffer
    dlp::Time::Sleep::Milliseconds(50);

    // Begin capturing images for camera calibration
    dlp::Image          camera_printed_board;
    dlp::Image::Window  camera_view;

    // Open the camera view
    camera_view.Open("Camera Calibration - press SPACE to capture or ESC to exit");

    // Capture images until enough boards have been successfully added
    while(boards_success < boards_required){

        // Wait for the space bar to add the calibration image
        unsigned int return_key = 0;
        while(return_key != ' '){
            camera_printed_board.Clear();               // Clear the image object
            camera->GetFrame(&camera_printed_board);    // Grab the latest camera frame
            camera_view.Update(camera_printed_board);   // Display the image
            camera_view.WaitForKey(16,&return_key);     // Wait for a key to be pressed or a 50ms timeout
            if(return_key == 27) break;                 // ESC key was pressed
        }

        // ESC key was pressed
        if(return_key == 27){

            // Close the image window
            camera_view.Close();

            // Stop the camera capture
            if(camera->Stop().hasErrors()){
                dlp::CmdLine::Print("Camera failed to stop! Exiting calibration routine...");
            }

            // Exit the calibration function
            dlp::CmdLine::Print("Exiting camera calibration...");
            return;
        }

        // Add the calibraiton board image
        bool success = false;
        camera_printed_board.ConvertToMonochrome(); // Convert the image to monochrome
        camera_calib.AddCalibrationBoard(camera_printed_board,&success);

        // Update the status
        camera_calib.GetCalibrationProgress(&boards_success,&boards_required);

        // Display message
        if(success){
            dlp::CmdLine::Print("Calibration board successfully added! Captured " + dlp::Number::ToString(boards_success) + " of "  + dlp::Number::ToString(boards_required));
            camera_printed_board.Save(camera_calib_image_file_names + dlp::Number::ToString(boards_success) + ".bmp");
        }
        else{
            dlp::CmdLine::Print("\nCalibration board NOT found! Please check the following:");
            dlp::CmdLine::Print("- All internal corners of the calibration board are visible");
            dlp::CmdLine::Print("  within the camera frame");
            dlp::CmdLine::Print("- The calibration board does not have any glare");
            dlp::CmdLine::Print("- The calibration board is well illuminated");
            dlp::CmdLine::Print("- The calibration board is in focus");
            dlp::CmdLine::Print();
        }
    }

    // Release the image memory
    camera_printed_board.Clear();

    // Calibrate the camera
    double reprojection_error;
    ret = camera_calib.Calibrate(&reprojection_error);
    if(!ret.hasErrors()){
        dlp::CmdLine::Print("Camera was successfully calibrated!");
        dlp::CmdLine::Print("\nReprojection error (closer to zero is better) = ", reprojection_error);

        // Get the camera calibration
        camera_calib.GetCalibrationData(&camera_calib_data);

        // Save the camera calibration data
        camera_calib_data.Save(camera_calib_data_file);
        dlp::CmdLine::Print("Camera calibration data saved to :", camera_calib_data_file);
    }
    else{
        dlp::CmdLine::Print("Camera calibration FAILED!\n");
    }

    return;
}

void CalibrateSystem(dlp::Camera       *camera,
                     const std::string &camera_calib_settings_file,
                     const std::string &camera_calib_data_file,
                     dlp::DLP_Platform *projector,
                     const std::string &projector_calib_settings_file,
                     const std::string &projector_calib_data_file,
                     const std::string &calib_image_file_names){

    dlp::ReturnCode ret;

    // Camera Calibration Variables
    dlp::Calibration::Camera    camera_calib;
    dlp::Calibration::Data      camera_calib_data;
    dlp::Parameters             camera_calib_settings;


    // Projector Calibration Variables
    dlp::Calibration::Projector projector_calib;
    dlp::Calibration::Data      projector_calib_data;
    dlp::Parameters             projector_calib_settings;

    camera_calib.SetDebugEnable(false);
    projector_calib.SetDebugEnable(false);

    // Check that camera is NOT null
    if(!camera) return;

    // Check if camera is connected
    if(!camera->isConnected()){
        dlp::CmdLine::Print("Camera NOT connected! \n");
        return;
    }

    // Check that projector is NOT null
    if(!projector) return;

    // Check if projector is connected
    if(!projector->isConnected()){
        dlp::CmdLine::Print("Projector NOT connected! \n");
        return;
    }

    // Check for camera calibration data
    bool camera_calib_exists = true;
    camera_calib_data.Load(camera_calib_data_file);
    if(camera_calib.SetCalibrationData(camera_calib_data).hasErrors()) camera_calib_exists = false;

    // Load camera calibration settings
    dlp::CmdLine::Print("Loading camera calibration settings...");
    if(camera_calib_settings.Load(camera_calib_settings_file).hasErrors()){

        // Camera calibration settings file did NOT load
        dlp::CmdLine::Print("Could NOT load camera calibration settings!");
        dlp::CmdLine::Print(ret.ToString());

        return;
    }

    // Load projector calibration settings
    dlp::CmdLine::Print("Loading projector calibration settings...");
    if(projector_calib_settings.Load(projector_calib_settings_file).hasErrors()){

        // Projector calibration settings file did NOT load
        dlp::CmdLine::Print("Could NOT load projector calibration settings!");
        dlp::CmdLine::Print(ret.ToString());

        return;
    }

    // Adjust camera calibration boards with the required number for the projector
    dlp::Calibration::Parameters::BoardCount projector_boards_required;
    projector_calib_settings.Get(&projector_boards_required);
    camera_calib_settings.Set(projector_boards_required);

    // Setup camera calibration for current camera
    camera_calib.SetCamera((*camera));
    projector_calib.SetCamera((*camera));
    projector_calib.SetDlpPlatform((*projector));

    dlp::CmdLine::Print("Setting up camera calibration...");
    ret = camera_calib.Setup(camera_calib_settings);
    if(ret.hasErrors()){
        dlp::CmdLine::Print("Camera calibration setup FAILED: ");
        dlp::CmdLine::Print(ret.ToString());
        return;
    }

    dlp::CmdLine::Print("Setting up projector calibration...");
    ret = projector_calib.Setup(projector_calib_settings);
    if(ret.hasErrors()){
        dlp::CmdLine::Print("Projector calibration setup FAILED: ");
        dlp::CmdLine::Print(ret.ToString());
        return;
    }

    // Start camera if it hasn't been started
    if(!camera->isStarted()){
        if(camera->Start().hasErrors()){
            dlp::CmdLine::Print("Could NOT start camera! \n");
            return;
        }
    }

    // Project calibration pattern to focus projector
    ret = projector->DisplayPatternInSequence(0,true);
    if(ret.hasErrors()){
        dlp::CmdLine::Print(ret.ToString());
        return;
    }

    dlp::Time::Sleep::Milliseconds(100);
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("Place the calibration board at the desired scanning");
    dlp::CmdLine::Print("distance and focus the projector...");
    dlp::CmdLine::Print();
    dlp::CmdLine::PressEnterToContinue("Press ENTER after the projector has been focused...");


    // Get the number of calibration images needed
    unsigned int cam_boards_success;
    unsigned int cam_boards_required;
    unsigned int proj_boards_success;
    unsigned int proj_boards_required;

    camera_calib.GetCalibrationProgress(&cam_boards_success,&cam_boards_required);
    projector_calib.GetCalibrationProgress(&proj_boards_success,&proj_boards_required);

    // Display the calibration instructions
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("Read the following instructions in their entirety!");
    dlp::CmdLine::PressEnterToContinue("Press ENTER when ready to read the instructions...");
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("System calibration instructions:");
    dlp::CmdLine::Print("1. After the live view has started, place the calibration board at the");
    dlp::CmdLine::Print("   furthest desired scanning distance");
    dlp::CmdLine::Print("   - The entire projection must fit on the calibration board!");
    dlp::CmdLine::Print("2. Place the camera in a position so that the projection is completely visible");
    dlp::CmdLine::Print("   within the camera frame");
    dlp::CmdLine::Print("   - The board should be placed at the desired scanning distance");
    dlp::CmdLine::Print("3. Lock the projector and camera in place relative to each other if possible");
    dlp::CmdLine::Print("   - If the camera or projector move relative to each other, the system");
    dlp::CmdLine::Print("     calibration must be redone");
    dlp::CmdLine::Print("4. Select the live view window and press the SPACE_BAR key to capture");
    dlp::CmdLine::Print("   the calibration board's current position");
    dlp::CmdLine::Print("5. Move the calibration board and repeat step 4 until calibration is complete");
    dlp::CmdLine::Print("   - Capture images with the board at different distances from the projector");
    dlp::CmdLine::Print("   - The projected pattern must always fit on the calibration board");
    dlp::CmdLine::Print("   - Both the projected pattern and printed pattern must be viewable by the");
    dlp::CmdLine::Print("     camera");
    dlp::CmdLine::Print("   - The position of the camera and projector relative to each other should");
    dlp::CmdLine::Print("     NOT be changed!");
    dlp::CmdLine::Print("\nNOTE: Press ESC key to quit the calibration routine");
    dlp::CmdLine::Print();
    dlp::CmdLine::PressEnterToContinue("Press ENTER after reading the above instructions...");

    // Begin capturing images for camera calibration
    dlp::Image          camera_printed_board;
    dlp::Image          projector_camera_combo;
    dlp::Image::Window  camera_view;
    unsigned int        return_key = 0;

    // Open the camera live view
    camera_view.Open("System Calibration - press SPACE to capture or ESC to exit");

    while(proj_boards_success < proj_boards_required){

        // Get the camera board first
        while(cam_boards_success < cam_boards_required){

            // Project white pattern to illuminate camera calibration board
            projector->ProjectSolidWhitePattern();
            dlp::Time::Sleep::Milliseconds(250);

            // Wait for the space bar to add the calibration image
            while(return_key != ' '){
                camera_printed_board.Clear();
                camera->GetFrame(&camera_printed_board);
                camera_view.Update(camera_printed_board);
                camera_view.WaitForKey(16,&return_key);
                if(return_key == 27) break;
            }

            if(return_key == 27){
                camera_view.Close();
                if(camera->Stop().hasErrors()){
                    dlp::CmdLine::Print("Camera failed to stop!");
                }
                dlp::CmdLine::Print("Exiting calibration...");
                return;
            }

            // Reset return key
            return_key = 0;

            // Add the calibration board image
            bool success;
            camera_printed_board.ConvertToMonochrome();
            camera_calib.AddCalibrationBoard(camera_printed_board,&success);

            // Update the status
            camera_calib.GetCalibrationProgress(&cam_boards_success,&cam_boards_required);

            // Display message
            if(success){
                dlp::CmdLine::Print("Camera calibration board successfully added! Captured " + dlp::Number::ToString(cam_boards_success) + " of " + dlp::Number::ToString(cam_boards_required));
                camera_printed_board.Save(calib_image_file_names + "camera_" + dlp::Number::ToString(cam_boards_success) + ".bmp");

                // Get the projector calibration capture now
                projector->DisplayPatternInSequence(0,true);
                dlp::Time::Sleep::Milliseconds(250);

                camera->GetFrame(&projector_camera_combo);
                camera_view.Update(projector_camera_combo);
                camera_view.WaitForKey(1,&return_key);

                dlp::Image projector_black;
                projector->ProjectSolidBlackPattern();
                dlp::Time::Sleep::Milliseconds(250);

                camera->GetFrame(&projector_black);
                camera_view.Update(projector_black);
                camera_view.WaitForKey(1,&return_key);


                // Add the printed and combination boards
                dlp::Image projector_pattern;
                projector_camera_combo.ConvertToMonochrome();
                projector_black.ConvertToMonochrome();

                projector_calib.RemovePrinted_AddProjectedBoard(camera_printed_board,
                                                                projector_black,
                                                                projector_camera_combo,
                                                                &projector_pattern,
                                                                &success);
				dlp::CmdLine::Print(" zk-test");
//				success = true;////////////zk  test
                projector_black.Clear();

                // Update the status
                projector_calib.GetCalibrationProgress(&proj_boards_success,&proj_boards_required);//zk  test
//				proj_boards_success++;	//zk  test
                if(success){
                    dlp::CmdLine::Print("Projector calibration board successfully added! Captured "  + dlp::Number::ToString(proj_boards_success) + " of " + dlp::Number::ToString(proj_boards_required));
                    projector_pattern.Save(calib_image_file_names + "projector_pattern_" + dlp::Number::ToString(proj_boards_success) +".bmp");
                    projector_camera_combo.Save(calib_image_file_names + "projector_camera_combo_" + dlp::Number::ToString(proj_boards_success) +".bmp");
                }
                else{
                    dlp::CmdLine::Print("Projector calibration board NOT found! Please check the following:");
                    dlp::CmdLine::Print("- All internal corners of the projected calibration board");
                    dlp::CmdLine::Print("  are visible within the camera frame");
                    dlp::CmdLine::Print("- The projector calibration board does NOT fall off of the");
                    dlp::CmdLine::Print("  printed calibration board");
                    dlp::CmdLine::Print("- The calibration board does not have any glare");
                    dlp::CmdLine::Print("- The calibration board is well illuminated");
                    dlp::CmdLine::Print("- The calibration board is in focus");
                    dlp::CmdLine::Print();
					projector_pattern.Save(calib_image_file_names + "projector_failed_pattern_" + dlp::Number::ToString(proj_boards_success) + ".bmp");
					projector_camera_combo.Save(calib_image_file_names + "projector_camera_failed_combo_" + dlp::Number::ToString(proj_boards_success) + ".bmp");
                    // Remove the most recently added camera calibration board
                    camera_calib.RemoveLastCalibrationBoard();
                    camera_calib.GetCalibrationProgress(&cam_boards_success,&cam_boards_required);
                }
            }
            else{
                dlp::CmdLine::Print("Calibration board NOT found! Please check the following:");
                dlp::CmdLine::Print("- All internal corners of the calibration board are visible");
                dlp::CmdLine::Print("  within the camera frame");
                dlp::CmdLine::Print("- The calibration board does not have any glare");
                dlp::CmdLine::Print("- The calibration board is well illuminated");
                dlp::CmdLine::Print("- The calibration board is in focus");
                dlp::CmdLine::Print();
            }
        }
    }

    // Release the image memory
    camera_printed_board.Clear();

    // Close the window
    camera_view.Close();

    // Calibrate the camera
    double error;
    int camera_calib_option = 0;

    if(camera_calib_exists){
        dlp::CmdLine::Print();
        dlp::CmdLine::Print("Calibration Options:");
        dlp::CmdLine::Print("0 - Update camera extrinsics (RECOMMENDED)");
        dlp::CmdLine::Print("    + Uses a previously completed camera only calibration");
        dlp::CmdLine::Print("1 - Update camera extrinsics, intrinsics, and distortion");
        dlp::CmdLine::Print("    + Uses the camera calibration done with the system calibration");
        dlp::CmdLine::Print();

        if(!dlp::CmdLine::Get(camera_calib_option,"Select option: ")) camera_calib_option = 0;
        if(camera_calib_option > 1) camera_calib_option = 0;
    }
    else{
        camera_calib_option = 1;
    }


    dlp::CmdLine::Print("Calibrating camera... \n");

    bool update_intrinsic   = false;
    bool update_distortion  = false;
    bool update_extrinsic   = false;
    switch (camera_calib_option) {
    case 0:
        update_intrinsic   = false;
        update_distortion  = false;
        update_extrinsic   = true;
        break;
    case 1:
        update_intrinsic   = true;
        update_distortion  = true;
        update_extrinsic   = true;
        break;
    }

    // Perform calibration calculations
    camera_calib.Calibrate(&error,update_intrinsic,update_distortion,update_extrinsic);

    if( ret.hasErrors()){
        if(camera->Stop().hasErrors()){
            dlp::CmdLine::Print("Camera failed to stop! Exiting calibration routine...");
        }
        dlp::CmdLine::Print("Camera calibration FAILED!");
        return;
    }

	dlp::CmdLine::Print("zk test");
    dlp::CmdLine::Print("Camera calibration succeeded with reprojection error (closer to zero is better) = ", error);

    // Get the camera calibration
    camera_calib.GetCalibrationData(&camera_calib_data);

    // Add camera calibration to projector calibration
    dlp::CmdLine::Print("Loading camera calibration into projector calibration...");
    projector_calib.SetCameraCalibration(camera_calib_data);

    dlp::CmdLine::Print("Calibrating projector... \n");
    ret = projector_calib.Calibrate(&error);

    if( ret.hasErrors()){
        if(camera->Stop().hasErrors()){
            dlp::CmdLine::Print("Camera failed to stop! Exiting calibration routine...");
        }
        dlp::CmdLine::Print("Projector calibration FAILED \n");
		dlp::CmdLine::Print("zk test");
        return;
    }

    dlp::CmdLine::Print("Projector calibration succeeded with reprojection error (closer to zero is better) = ", error);

    // Get the projector and camera calibration
    projector_calib.GetCalibrationData(&projector_calib_data);

    // Save the calibration data
    dlp::CmdLine::Print("Saving camera and projector calibration data...");
    camera_calib_data.Save(camera_calib_data_file);
    projector_calib_data.Save(projector_calib_data_file);

    if(camera->Stop().hasErrors()){
        dlp::CmdLine::Print("Camera failed to stop! Exiting calibration routine...");
    }
    dlp::CmdLine::Print("Calibration data saved");

    return;
}

void ScanObject(dlp::Camera          *camera,
                const bool           &cam_proj_hw_synchronized,
                const std::string    &camera_calib_data_file,
                dlp::DLP_Platform    *projector,
                const std::string    &projector_calib_data_file,
                dlp::StructuredLight *structured_light_vertical,
                dlp::StructuredLight *structured_light_horizontal,
                const bool           &use_vertical,
                const bool           &use_horizontal,
                const std::string    &geometry_settings_file,
                const bool           &continuous_scanning,
				int					 scan_times=1,
				int					 stop_time_ms=0	){


				
		//更改扫描次数
		char t[10];
		std::cout<<"输入一周旋转几次：" << std::endl;
		std::cin >> t;
		scan_times=atoi(t);
				
    //进行串口连接
	//zk_uart_test
		dlp::CmdLine::Print("连接单片机 ");
	
	    HANDLE hcom;
		hcom = CreateFile("COM4",GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING 
								,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hcom == INVALID_HANDLE_VALUE)
		{
			dlp::CmdLine::Print("连接失败 ");
		}
		SetupComm(hcom,1024,1024);
		DCB dcb;
		GetCommState(hcom,&dcb);
		dcb.BaudRate = 4800;
		dcb.ByteSize = 8;
		dcb.Parity = 0;
		dcb.StopBits = 1;
		SetCommState(hcom,&dcb);

	    char data[2];
		data[0]=(char)scan_times;
		data[1]='a';
		
    // Check that camera is NOT null
    if(!camera) return;

    // Check if camera is connected
    if(!camera->isConnected()){
        dlp::CmdLine::Print("Camera NOT connected! \n");
        return;
    }

    // Check that projector is NOT null
    if(!projector) return;

    // Check if projector is connected
    if(!projector->isConnected()){
        dlp::CmdLine::Print("Projector NOT connected! \n");
        return;
    }

    // Check that structured light module pointers are valid
    if(!structured_light_vertical)   return;
    if(!structured_light_horizontal) return;

    if(use_vertical){
        if(!structured_light_vertical->isSetup()){
            dlp::CmdLine::Print("Vertical structured light module NOT setup! \n");
            return;
        }
    }

    if(use_horizontal){
        if(!structured_light_horizontal->isSetup()){
            dlp::CmdLine::Print("Horizontal structured light module NOT setup! \n");
            return;
        }
    }

    // Check that calibrations are complete
    dlp::Calibration::Data calibration_data_camera;
    dlp::Calibration::Data calibration_data_projector;


    dlp::CmdLine::Print("Loading camera and projector calibration data...");
    calibration_data_camera.Load(camera_calib_data_file);
    calibration_data_projector.Load(projector_calib_data_file);

    if(!calibration_data_camera.isComplete()){
        dlp::CmdLine::Print("Camera calibration is NOT complete! \n");
        return;
    }

    if(!calibration_data_camera.isCamera()){
        dlp::CmdLine::Print("Camera calibration is NOT from a camera calibration! \n");
        return;
    }

    if(!calibration_data_projector.isComplete()){
        dlp::CmdLine::Print("Projector calibration is NOT complete! \n");
        return;
    }

    if(calibration_data_projector.isCamera()){
        dlp::CmdLine::Print("Projector calibration is NOT from a projector calibration! \n");
        return;
    }

    // Check that the camera resolution matches the camera calibration data resolution
    unsigned int calibration_camera_rows;
    unsigned int calibration_camera_columns;
    unsigned int camera_rows;
    unsigned int camera_columns;
    calibration_data_camera.GetModelResolution(&calibration_camera_columns,&calibration_camera_rows);
    camera->GetColumns(&camera_columns);
    camera->GetRows(&camera_rows);

    if((calibration_camera_columns != camera_columns) ||
       (calibration_camera_rows    != camera_rows)){
        dlp::CmdLine::Print("\nThe camera calibration data model resolution and camera resolution do NOT match!");
        dlp::CmdLine::Print("Please use the calibration files for this specific camera!");
        std::cout << "Camera resolution from Calibration File  - (" << calibration_camera_columns << "," << calibration_camera_rows << ") " << "Actual Camera resolution - (" << camera_columns << "," << camera_rows << ") " << std::endl;
        dlp::CmdLine::PressEnterToContinue("Press ENTER to continue...");
        return;
    }


    // Check that the projector resolution matches the projector calibration data resolution
    unsigned int calibration_projector_rows;
    unsigned int calibration_projector_columns;
    unsigned int projector_rows;
    unsigned int projector_columns;
    calibration_data_projector.GetModelResolution(&calibration_projector_columns,&calibration_projector_rows);
    projector->GetColumns(&projector_columns);
    projector->GetRows(&projector_rows);

    if((calibration_projector_columns != projector_columns) ||
       (calibration_projector_rows    != projector_rows)){
        dlp::CmdLine::Print("\nThe projector calibration data model resolution and projector resolution do NOT match!");
        dlp::CmdLine::Print("Please use the calibration files for this specific projector!");
        std::cout << "Projector resolution from Calibration File  - (" << calibration_projector_columns << "," << calibration_projector_rows << ") " << "Actual Camera resolution - (" << projector_columns << "," << projector_rows << ") " << std::endl;
        dlp::CmdLine::PressEnterToContinue("Press ENTER to continue...");
        return;
    }

    dlp::Parameters geometry_settings;
	dlp::Geometry scanner_geometry;
    geometry_settings.Load(geometry_settings_file);
    scanner_geometry.Setup(geometry_settings);

	unsigned int camera_viewport;

	dlp::CmdLine::Print("Constructing the camera and projector geometry...");
		
	scanner_geometry.SetDebugEnable(false);
	scanner_geometry.SetOriginView(calibration_data_projector);
	scanner_geometry.AddView(calibration_data_camera, &camera_viewport);
	
    // Variables for viewers during the scan
    dlp::Point::Cloud::Window view_point_cloud;

    // Variables used during scan loop
    int scan_count = 0;
    std::atomic_bool continue_scanning(true);
    dlp::Time::Chronograph  timer;
    dlp::Capture::Sequence  capture_scan;
    dlp::Point::Cloud       point_cloud;
    dlp::Image depth_map;
    dlp::Image color_map;


    // Get the camera frame rate (This assumes the camera triggers the projector!)
    float frame_rate;
    camera->GetFrameRate(&frame_rate);

    // Determine the total scan time
    unsigned int period_us = 1000000 / frame_rate;
    unsigned int vertical_pattern_count   = structured_light_vertical->GetTotalPatternCount();
    unsigned int horizontal_pattern_count = structured_light_horizontal->GetTotalPatternCount();
    unsigned int capture_time = 0;
    unsigned int pattern_start = 1; // There is one calibration image so start with offset
    unsigned int pattern_count = 0;

    if(use_vertical){
        pattern_count += vertical_pattern_count;
    }
    else{
        pattern_start += vertical_pattern_count;
    }

    if(use_horizontal){
        pattern_count += horizontal_pattern_count;
    }

    capture_time += period_us * pattern_count;

    // Open a camera view so that the target object can be placed
    // within the view of the camera and projector
    // Begin capturing images for camera calibration
    dlp::Image          camera_frame;
    dlp::Image::Window  camera_view;

    // Open the camera view
    dlp::CmdLine::Print("\nPlace the scanning target within the view of the camera and projector. \nPress SPACE or ESC from window when ready to scan...");
    camera_view.Open("Place Target in View - press SPACE or ESC to scan");

    // Start capturing images from the camera
    if(camera->Start().hasErrors()){
        dlp::CmdLine::Print("Could NOT start camera! \n");
        return;
    }

    // Project white so the object can be placed correctly
    projector->ProjectSolidWhitePattern();

    // Wait for the space bar or ESC key to be pressed before scanning
    unsigned int return_key = 0;
    while(return_key != ' '){
        camera_frame.Clear();               // Clear the image object
        camera->GetFrame(&camera_frame);    // Grab the latest camera frame
        camera_view.Update(camera_frame);   // Display the image
        camera_view.WaitForKey(16,&return_key);     // Wait for a key to be pressed or a 50ms timeout
        if(return_key == 27) break;                 // ESC key was pressed
    }

    // Close the image window
    camera_view.Close();


    // Display the instructions to use the point cloud viewer
    dlp::CmdLine::Print();
    dlp::CmdLine::Print("Point Cloud Viewer Operation:");
    dlp::CmdLine::Print("i/I = Zoom in");
    dlp::CmdLine::Print("o/O = Zoom out");
    dlp::CmdLine::Print("s/S = Save point cloud xyz file");
    dlp::CmdLine::Print("a/A = Auto-rotate the point cloud");
    dlp::CmdLine::Print("c/C = Turn point cloud color on/off");
    dlp::CmdLine::Print("\nNOTE: Press ESC key to quit the scan routine");
    dlp::CmdLine::Print();
//    dlp::CmdLine::PressEnterToContinue("Press ENTER after reading the above instructions...");


    // Open the point cloud viewer
    view_point_cloud.Open("Point Cloud Viewer (color based on z value) - press ESC to stop scanning...",600,400);

    // Enter the scanning loop
	while (scan_times){

		dlp::CmdLine::Print("\nStarting scan ", scan_count, "...");

		dlp::Capture::Sequence vertical_scan;
		dlp::Capture::Sequence horizontal_scan;

		//Peform images capture when both camera and projector are connected
		//via HW trigger signal for synchronization
		if (cam_proj_hw_synchronized == true) {

			// Start capturing images from the camera
			if (camera->Start().hasErrors()){
				dlp::CmdLine::Print("Could NOT start camera! \n");
				return;
			}

			// Give camera time to start capturing images
			dlp::Time::Sleep::Milliseconds(100);

			// Scan the object
			dlp::ReturnCode sequence_return;
			sequence_return = projector->StartPatternSequence(pattern_start, pattern_count, false);
			if (sequence_return.hasErrors()){
				dlp::CmdLine::Print("Sequence failed! Exiting scan routine...");
				if (camera->Stop().hasErrors()){
					dlp::CmdLine::Print("Camera failed to stop! Exiting scan routine...");
				}
				dlp::CmdLine::Print("Sequence failed..." + sequence_return.ToString());
				return;
			}

			timer.Reset();

			// Wait for the sequence to finish and add a little extra time
			// to account for the sequence validation time required
			dlp::Time::Sleep::Microseconds(capture_time + 10 * period_us);

			// Stop grabbing images from the camera
			if (camera->Stop().hasErrors()){
				dlp::CmdLine::Print("Camera failed to stop! Exiting scan routine...");
				return;
			}
			dlp::CmdLine::Print("Pattern sequence capture completed in...\t", timer.Lap(), "ms");
			projector->StopPatternSequence();

			// Grab all of the images from the buffer to find the pattern sequence
			bool            min_images = false;
			dlp::ReturnCode ret;
			dlp::Capture    capture;
			dlp::Image      capture_image;

			unsigned int iPattern = 0;

			std::vector<double> capture_sums;

			while (!min_images){

				capture_image.Clear();

				ret = camera->GetFrameBuffered(&capture_image);
				iPattern++;
				capture_image.ConvertToMonochrome();
				if (ret.hasErrors()){
					min_images = true;
				}
				else{

					double sum = 0;
					capture_image.GetSum(&sum);
					capture_sums.push_back(sum);

					// Add the frame to the sequence
					capture.image_data = capture_image;
					capture.data_type = dlp::Capture::DataType::IMAGE_DATA;
					capture_scan.Add(capture);
					capture_image.Clear();
					capture.image_data.Clear();

				}

			}

			dlp::CmdLine::Print("Images retreived from buffer in...\t\t", timer.Lap(), "ms");

			// Find  the first pattern and the seperate the vertical and horizontal patterns
			vertical_scan.Clear();
			horizontal_scan.Clear();

			bool first_pattern_found = false;
			unsigned int capture_offset = 0;
			unsigned int vertical_patterns_added = 0;
			unsigned int horizontal_patterns_added = 0;
			double previous_sum = 0;

			timer.Lap();

			for (unsigned int iScan = 0; iScan < capture_scan.GetCount(); iScan++){

				// Calculate the average for the image to determine if the pattern started
				if (!first_pattern_found){

					// Retrieve the image sum value
					double sum = capture_sums.at(iScan);

					if (iScan == 0){
						previous_sum = sum;
					}
					else if (previous_sum != 0)
					{
						// If the percent error is greater than 10% the first pattern has been found
						if (sum > (previous_sum * 1.1)){
							first_pattern_found = true;
							capture_offset = iScan;
						}
						previous_sum = sum;
					}
				}

				if (first_pattern_found){
					if ((capture_offset + pattern_count) < capture_scan.GetCount()){

						dlp::Capture temp;
						capture_scan.Get(iScan, &temp);


						if (use_vertical && (vertical_patterns_added < vertical_pattern_count)){
							// Add vertical patterns
							vertical_scan.Add(temp);
							temp.image_data.Save("output/scan_images/scan_capture_" + dlp::Number::ToString(iScan - capture_offset) + ".bmp");
							vertical_patterns_added++;
						}
						else if (use_horizontal && (horizontal_patterns_added < horizontal_pattern_count)){
							// Add horizontal patterns
							horizontal_scan.Add(temp);
							temp.image_data.Save("output/scan_images/scan_capture_" + dlp::Number::ToString(iScan - capture_offset) + ".bmp");
							horizontal_patterns_added++;
						}

						temp.image_data.Clear();
					}
				}
			}
		}
		else {
			//Perform images capture with camera is in free running mode i.e.,
			//if they are not synchronized via HW trigger signal
			// Project a black screen
			projector->ProjectSolidBlackPattern();
			dlp::Time::Sleep::Milliseconds(100);

			// Start capturing images from the camera
			if (camera->Start().hasErrors()){
				dlp::CmdLine::Print("Could NOT start camera! \n");
				return;
			}

			timer.Reset();

			// Grab all of the images from the buffer to find the pattern sequence
			bool            min_images = false;
			dlp::ReturnCode ret;
			dlp::Capture    capture;
			dlp::Image      capture_image;

			unsigned int iPattern = 0;

			while (!min_images){

				capture_image.Clear();

				// Display each pattern
				projector->DisplayPatternInSequence(pattern_start + iPattern, true);
				iPattern++;

				dlp::Time::Sleep::Milliseconds(100);

				// Grab the most recent camera image
				ret = camera->GetFrame(&capture_image);
				capture_image.ConvertToMonochrome();

				if (ret.hasErrors()){
					// Check if the buffer is empty
					if (ret.ContainsError(OPENCV_CAM_IMAGE_BUFFER_EMPTY)){
						min_images = true;
					}
				}
				else{
					// Add the frame to the sequence
					capture.image_data = capture_image;
					capture.data_type = dlp::Capture::DataType::IMAGE_DATA;
					capture_scan.Add(capture);
					capture_image.Clear();
					capture.image_data.Clear();
				}

				if (iPattern == pattern_count) min_images = true;
			}

			dlp::CmdLine::Print("Pattern sequence capture completed in...\t", timer.Lap(), "ms");

			// Restart the camera so that the white pattern will display during processing
			projector->ProjectSolidWhitePattern();
			camera->Start();

			// Find  the first pattern and the seperate the vertical and horizontal patterns
			vertical_scan.Clear();
			horizontal_scan.Clear();

			unsigned int capture_offset = 0;
			unsigned int vertical_patterns_added = 0;
			unsigned int horizontal_patterns_added = 0;

			timer.Lap();

			for (unsigned int iScan = 0; iScan < capture_scan.GetCount(); iScan++){
				dlp::Capture temp;
				capture_scan.Get(iScan, &temp);

				if (use_vertical && (vertical_patterns_added < vertical_pattern_count)){
					// Add vertical patterns
					vertical_scan.Add(temp);
					temp.image_data.Save("output/scan_images/scan_capture_" + dlp::Number::ToString(iScan - capture_offset) + ".bmp");
					vertical_patterns_added++;
				}
				else if (use_horizontal && (horizontal_patterns_added < horizontal_pattern_count)){
					// Add horizontal patterns
					horizontal_scan.Add(temp);
					temp.image_data.Save("output/scan_images/scan_capture_" + dlp::Number::ToString(iScan - capture_offset) + ".bmp");
					horizontal_patterns_added++;
				}

				temp.image_data.Clear();
			}

		}

		capture_scan.Clear();
		dlp::CmdLine::Print("Patterns sorted in...\t\t\t\t", timer.Lap(), "ms");

		dlp::DisparityMap column_disparity;
		dlp::DisparityMap row_disparity;
		dlp::Point::Cloud point_cloud_new;

		column_disparity.Clear();
		row_disparity.Clear();

		if (use_vertical && (vertical_pattern_count == vertical_scan.GetCount())){
			timer.Lap();
			structured_light_vertical->DecodeCaptureSequence(&vertical_scan, &column_disparity);
			dlp::CmdLine::Print("Vertical patterns decoded in...\t\t\t", timer.Lap(), "ms");
		}


		if (use_horizontal && (horizontal_pattern_count == horizontal_scan.GetCount())){
			timer.Lap();
			structured_light_horizontal->DecodeCaptureSequence(&horizontal_scan, &row_disparity);
			dlp::CmdLine::Print("Horizontal patterns decoded in...\t\t", timer.Lap(), "ms");
		}

		if (use_vertical && (!use_horizontal)){
			// Use vertical patterns only

			// Check that there are enough patterns to decode
			if (vertical_pattern_count != vertical_scan.GetCount()){
				dlp::CmdLine::Print("NOT enough images. Scans may have been too dark. Please rescan. \n");
			}
			else{
				scan_count++;
				point_cloud_new.Clear();
				scanner_geometry.GeneratePointCloud(camera_viewport, column_disparity, &point_cloud, &depth_map);
				dlp::CmdLine::Print("Point cloud reconstructed in...\t\t\t", timer.Lap(), "ms");
			}
		}
		else if (use_horizontal && (!use_vertical)){
			// Use horizontal patterns only

			// Check that there are enough patterns to decode
			if (horizontal_pattern_count != horizontal_scan.GetCount()){
				dlp::CmdLine::Print("NOT enough images. Scans may have been too dark. Please rescan. \n");
			}
			else{
				scan_count++;
				point_cloud_new.Clear();
				scanner_geometry.GeneratePointCloud(camera_viewport, row_disparity, &point_cloud, &depth_map);
				dlp::CmdLine::Print("Point cloud reconstructed in...\t\t\t", timer.Lap(), "ms");
			}
		}
		else if (use_vertical && use_horizontal){
			// Use both vertical and horizontal

			// Check that there are enough patterns to decode
			if ((structured_light_vertical->GetTotalPatternCount() != vertical_scan.GetCount()) ||
				(structured_light_horizontal->GetTotalPatternCount() != horizontal_scan.GetCount())){
				dlp::CmdLine::Print("NOT enough images. Scans may have been too dark. Please rescan. \n");
			}
			else{
				scan_count++;
				point_cloud_new.Clear();
				scanner_geometry.GeneratePointCloud(camera_viewport, column_disparity, row_disparity, &point_cloud, &depth_map);
				dlp::CmdLine::Print("Point cloud reconstructed in...\t\t\t", timer.Lap(), "ms");
			}
		}

		// Update viewers
		view_point_cloud.Update(point_cloud);

		// Check if the point cloud viewer is open or the scan should
		// only be performed once

		// Clear variables
		vertical_scan.Clear();
		horizontal_scan.Clear();
		column_disparity.Clear();
		row_disparity.Clear();


		// Wait for the view to close
		dlp::CmdLine::Print("\nWaiting for point cloud viewer to close...");
		//while(view_point_cloud.isOpen()){};


		//view_point_cloud.Close();

		// Check if most recent scan should be saved
		dlp::CmdLine::Print();
		dlp::CmdLine::Print("Save most recent scan");
		//dlp::CmdLine::Print("0 - Do NOT save files");
		//dlp::CmdLine::Print("1 - Save point cloud data and color depth map");
		dlp::CmdLine::Print();

		unsigned int save_data = 0;
		//if(!dlp::CmdLine::Get(save_data,"Select option: ")) save_data = 0;
		save_data = 1;//zk
		if (save_data == 1){
			std::string file_time = dlp::Number::ToString((int)(data[0])+1-scan_times);//zk

			dlp::CmdLine::Print();
			dlp::CmdLine::Print("Saving depth color map...");
			dlp::Geometry::ConvertDistanceMapToColor(depth_map, &color_map);
			color_map.Save("output/scan_data/" + file_time + "_color_map.bmp");

			dlp::CmdLine::Print("Saving point cloud...");
			point_cloud.SaveXYZ("output/scan_data/" + file_time + "_point_cloud.xyz", ' ');
		}

		if (camera->Stop().hasErrors()){
			dlp::CmdLine::Print("Camera failed to stop! Exiting scan routine...");
		}
		continue_scanning = view_point_cloud.isOpen() & continuous_scanning;
		scan_times--;
		dlp::CmdLine::Print("旋转等待...");
		
		DWORD dwWrittenLen = 0;
		if(!WriteFile(hcom,data,1,&dwWrittenLen,NULL))
        {
         dlp::CmdLine::Print("发送失败 ");
        }

		_sleep(1000);

		char str[2];
		char aa,bb;
		DWORD wCount;//读取的字节数		 
		BOOL bReadStat;		 
		bReadStat=ReadFile(hcom,str,1,&wCount,NULL);
		if(!bReadStat)
        {
         dlp::CmdLine::Print("读取失败 ");
        }
		aa=str[0];
	//	bb=str[1];
		dlp::CmdLine::Print("接收:",aa);		
	//	dlp::CmdLine::Print("接收:",bb);
		_sleep(stop_time_ms*8/((int)(data[0])));
	}
    // Close the viewers
    view_point_cloud.Close();

    // Clear the geometry module to release memory
    scanner_geometry.Clear();

    return;
}

int main()
{
    // Configuration Parameter Definitions

    //Camera type: 0 - Generic OpenCV camera, 1 - PointGrey, ...
    DLP_NEW_PARAMETERS_ENTRY(CameraType,         "CAMERA_TYPE",  int, -1);
    //Algorithm type: 0 - Graycode, 1 - Hybrid ThreePhase, ...
    DLP_NEW_PARAMETERS_ENTRY(AlgorithmType,      "ALGORITHM_TYPE",  int, -1);

    DLP_NEW_PARAMETERS_ENTRY(ConnectIdProjector,           "CONNECT_ID_PROJECTOR",  std::string, "0");
    DLP_NEW_PARAMETERS_ENTRY(ConnectIdCamera,              "CONNECT_ID_CAMERA",     std::string, "0");

    DLP_NEW_PARAMETERS_ENTRY(ConfigFileProjector,           "CONFIG_FILE_PROJECTOR",                std::string, "config/config_projector.txt");
    DLP_NEW_PARAMETERS_ENTRY(ConfigFileCamera,              "CONFIG_FILE_CAMERA",                   std::string, "config/config_camera.txt");
    DLP_NEW_PARAMETERS_ENTRY(ConfigFileCalibProjector,      "CONFIG_FILE_CALIBRATION_PROJECTOR",    std::string, "config/calibration_projector.txt");
    DLP_NEW_PARAMETERS_ENTRY(ConfigFileCalibCamera,         "CONFIG_FILE_CALIBRATION_CAMERA",       std::string, "config/calibration_camera.txt");
    DLP_NEW_PARAMETERS_ENTRY(ConfigFileGeometry,            "CONFIG_FILE_GEOMETRY",                 std::string, "config/geometry.txt");
    DLP_NEW_PARAMETERS_ENTRY(ConfigFileStructuredLight1,    "CONFIG_FILE_STRUCTURED_LIGHT_1",       std::string, "config/algorithm_vertical.txt");
    DLP_NEW_PARAMETERS_ENTRY(ConfigFileStructuredLight2,    "CONFIG_FILE_STRUCTURED_LIGHT_2",       std::string, "config/algorithm_horizontal.txt");

    DLP_NEW_PARAMETERS_ENTRY(ContinuousScanning,            "CONTINUOUS_SCANNING", bool, false);

    DLP_NEW_PARAMETERS_ENTRY(CalibDataFileProjector,        "CALIBRATION_DATA_FILE_PROJECTOR",      std::string, "calibration/data/projector.xml");
    DLP_NEW_PARAMETERS_ENTRY(CalibDataFileCamera,           "CALIBRATION_DATA_FILE_CAMERA",         std::string, "calibration/data/camera.xml");
    DLP_NEW_PARAMETERS_ENTRY(DirCalibData,                  "DIRECTORY_CALIBRATION_DATA",                   std::string, "calibration/data/");
    DLP_NEW_PARAMETERS_ENTRY(DirCameraCalibImageOutput,     "DIRECTORY_CAMERA_CALIBRATION_IMAGE_OUTPUT",    std::string, "calibration/camera_images/");
    DLP_NEW_PARAMETERS_ENTRY(DirSystemCalibImageOutput,     "DIRECTORY_SYSTEM_CALIBRATION_IMAGE_OUTPUT",    std::string, "calibration/system_images/");
    DLP_NEW_PARAMETERS_ENTRY(DirScanDataOutput,             "DIRECTORY_SCAN_DATA_OUTPUT",                   std::string, "output/scan_data/");
    DLP_NEW_PARAMETERS_ENTRY(DirScanImagesOutput,           "DIRECTORY_SCAN_IMAGES_OUTPUT",                 std::string, "output/scan_images/");
    DLP_NEW_PARAMETERS_ENTRY(OutputNameImageCameraCalibBoard, "OUTPUT_NAME_IMAGE_CAMERA_CALIBRATION_BOARD", std::string, "camera_calibration_board");
    DLP_NEW_PARAMETERS_ENTRY(OutputNameImageCameraCalib,    "OUTPUT_NAME_IMAGE_CAMERA_CALIBRATION", std::string, "camera_calibration_capture_");
    DLP_NEW_PARAMETERS_ENTRY(OutputNameImageSystemCalib,    "OUTPUT_NAME_IMAGE_SYSTEM_CALIBRATION", std::string, "system_calibration_capture_");
    DLP_NEW_PARAMETERS_ENTRY(OutputNameImageDepthMap,       "OUTPUT_NAME_IMAGE_DEPTHMAP",           std::string, "_depthmap");
    DLP_NEW_PARAMETERS_ENTRY(OutputNameXYZPointCloud,       "OUTPUT_NAME_XYZ_POINTCLOUD",           std::string, "_pointcloud");

    // Configuration Paramters

    AlgorithmType               algorithm_type;
    CameraType                  camera_type;

    ConnectIdProjector          connect_id_projector;
    ConnectIdCamera             connect_id_camera;

    ConfigFileProjector         config_file_projector;
    ConfigFileCamera            config_file_camera;
    ConfigFileCalibProjector    config_file_calib_projector;
    ConfigFileCalibCamera       config_file_calib_camera;
    ConfigFileGeometry          config_file_geometry;
    ConfigFileStructuredLight1  config_file_structured_light_1;
    ConfigFileStructuredLight2  config_file_structured_light_2;

    ContinuousScanning          continuous_scanning;

    CalibDataFileProjector      calib_data_file_projector;
    CalibDataFileCamera         calib_data_file_camera;
    DirCalibData                dir_calib_data;
    DirCameraCalibImageOutput   dir_camera_calib_image_output;
    DirSystemCalibImageOutput   dir_system_calib_image_output;
    DirScanDataOutput           dir_scan_data_output;
    DirScanImagesOutput         dir_scan_images_output;

    OutputNameImageCameraCalibBoard output_name_image_camera_calib_board;
    OutputNameImageCameraCalib  output_name_image_camera_calib;
    OutputNameImageSystemCalib  output_name_image_system_calib;
    OutputNameImageDepthMap     output_name_image_depthmap;
    OutputNameXYZPointCloud     output_name_xyz_pointcloud;


    // Load the settings
    dlp::Parameters settings;
    settings.Load("DLP_LightCrafter_4500_3D_Scan_Application_Config.txt");

    // Retrieve the settings
    settings.Get(&algorithm_type);
    settings.Get(&camera_type);
    settings.Get(&connect_id_projector);
    settings.Get(&connect_id_camera);
    settings.Get(&config_file_projector);
    settings.Get(&config_file_camera);
    settings.Get(&config_file_calib_projector);
    settings.Get(&config_file_calib_camera);
    settings.Get(&config_file_geometry);
    settings.Get(&config_file_structured_light_1);
    settings.Get(&config_file_structured_light_2);
    settings.Get(&continuous_scanning);
    settings.Get(&calib_data_file_projector);
    settings.Get(&calib_data_file_camera);
    settings.Get(&dir_calib_data);
    settings.Get(&dir_camera_calib_image_output);
    settings.Get(&dir_system_calib_image_output);
    settings.Get(&dir_scan_data_output);
    settings.Get(&dir_scan_images_output);
    settings.Get(&output_name_image_camera_calib_board);
    settings.Get(&output_name_image_camera_calib);
    settings.Get(&output_name_image_system_calib);
    settings.Get(&output_name_image_depthmap);
    settings.Get(&output_name_xyz_pointcloud);

    // System Variables
    dlp::OpenCV_Cam     camera_cv;
    dlp::PG_FlyCap2_C   camera_pg;
    dlp::GrayCode       algo_gray_code_vert;
    dlp::GrayCode       algo_gray_code_horz;
    dlp::ThreePhase     algo_three_phase_vert;
    dlp::ThreePhase     algo_three_phase_horz;
    dlp::LCr4500        projector;
    unsigned int total_pattern_count = 0;

    // Validate the Camera and Algorithm types are within supported list
    if(camera_type.Get() > 1) {
        dlp::CmdLine::Print("Unsupported CAMERA_TYPE set in the configuration file. Modify DLP_LightCrafter_3D_Scan_Application_Config.txt");
        dlp::CmdLine::Print("Press any key to exit...");
        std::cin.get();
        return -1;
    }
    if(algorithm_type.Get() > 1) {
        dlp::CmdLine::Print("Unsupported ALGORITHM_TYPE set in the configuration file. Modify DLP_LightCrafter_3D_Scan_Application_Config.txt");
        dlp::CmdLine::Print("Press any key to exit...");
        std::cin.get();
        return -1;
    }
    // Connect camera and projector
    dlp::ReturnCode ret;

    ret = dlp::DLP_Platform::ConnectSetup(projector,connect_id_projector.Get(),config_file_projector.Get(),true);
    if(ret.hasErrors()) {
        dlp::CmdLine::Print("\n\nPlease resolve the LightCrafter connection issue before proceeding to next step...\n");
    }

    //Connect camera based on the user selection
    if(camera_type.Get() == 0) {
        ret = dlp::Camera::ConnectSetup(camera_cv,connect_id_camera.Get(),config_file_camera.Get(),true);
        if(ret.hasErrors()) {
            dlp::CmdLine::Print("\n\nPlease resolve the camera connection issue before proceeding to next step...\n");
        }
    } else if(camera_type.Get() == 1) {
        ret = dlp::Camera::ConnectSetup(camera_pg,connect_id_camera.Get(),config_file_camera.Get(),true);
        if(ret.hasErrors()) {
            dlp::CmdLine::Print("\n\nPlease resolve the camera connection issue before proceeding to next step...\n");
        }
    } else {
        //  unreachable code
    }

    // Program menu
    int menu_select = 0;

/*/zk_uart_test
	dlp::CmdLine::Print("kaishi ");

	    HANDLE hcom;
		hcom = CreateFile("COM4",GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING 
							,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hcom == INVALID_HANDLE_VALUE)
		{
			return -1;
		}
		SetupComm(hcom,1024,1024);
		DCB dcb;
		GetCommState(hcom,&dcb);
		dcb.BaudRate = 4800;
		dcb.ByteSize = 8;
		dcb.Parity = 0;
		dcb.StopBits = 1;
		SetCommState(hcom,&dcb);
		char data[]={'a','b', 'c', 'd', 'e', 'f','g','h'};
		DWORD dwWrittenLen = 0;
		if(!WriteFile(hcom,data,8,&dwWrittenLen,NULL))
        {
         dlp::CmdLine::Print("发送失败 ");
        }

		_sleep(1000);

		char str[8];
		char a;
		DWORD wCount;//读取的字节数		 
		BOOL bReadStat;		 
		bReadStat=ReadFile(hcom,str,8,&wCount,NULL);
		if(!bReadStat)
        {
         dlp::CmdLine::Print("读取失败 ");
        }
		a=str[2];
		dlp::CmdLine::Print("接收:",a);

		
		dlp::CmdLine::Print("jieshu");

/////////*/



		
    do{
        // Print menu
        dlp::CmdLine::Print();
        dlp::CmdLine::Print("Texas Instruments DLP Commandline 3D Scanner \n");
        dlp::CmdLine::Print("0: Exit ");
        dlp::CmdLine::Print("1: Generate camera calibration board and enter feature measurements ");
        dlp::CmdLine::Print("2: Prepare DLP LightCrafter 4500 (once per projector)");
        dlp::CmdLine::Print("3: Prepare system for calibration and scanning");
        dlp::CmdLine::Print("4: Calibrate camera ");
        dlp::CmdLine::Print("5: Calibrate system ");
        dlp::CmdLine::Print("6: Perform scan (vertical patterns only)");
        dlp::CmdLine::Print("7: Perform scan (horizontal patterns only)");
        dlp::CmdLine::Print("8: Perform scan (vertical and horizontal patterns)");
        dlp::CmdLine::Print("9: Reconnect camera and projector ");
        dlp::CmdLine::Print();

        // Get the menu item selection
        bool menu_select_valid = dlp::CmdLine::Get(menu_select,"Select menu item: ");


        // Check that value entered was an integer
        if(!menu_select_valid) menu_select = -1;

        // Execute selection
        switch(menu_select){
        case 0:
            break;
        case 1:
            if(camera_type.Get() == 0) {
                GenerateCameraCalibrationBoard(&camera_cv,
                                               config_file_calib_camera.Get(),
                                               dir_camera_calib_image_output.Get() +
                                               output_name_image_camera_calib_board.Get() +
                                               ".bmp");
            } else if(camera_type.Get() == 1) {
                GenerateCameraCalibrationBoard(&camera_pg,
                                               config_file_calib_camera.Get(),
                                               dir_camera_calib_image_output.Get() +
                                               output_name_image_camera_calib_board.Get() +
                                               ".bmp");
            } else {
                //  unreachable code
            }


            break;
        case 2:
            if(algorithm_type.Get() == 0) {
                PrepareProjectorPatterns(&projector,
                                         config_file_calib_projector.Get(),
                                         &algo_gray_code_vert,
                                         config_file_structured_light_1.Get(),
                                         &algo_gray_code_horz,
                                         config_file_structured_light_2.Get(),
                                         false,    // Firmware will be uploaded
                                         &total_pattern_count);
            } else if(algorithm_type.Get() == 1) {
                PrepareProjectorPatterns(&projector,
                                         config_file_calib_projector.Get(),
                                         &algo_three_phase_vert,
                                         config_file_structured_light_1.Get(),
                                         &algo_three_phase_horz,
                                         config_file_structured_light_2.Get(),
                                         false,    // Firmware will be uploaded
                                         &total_pattern_count);

            } else {
                //  unreachable code
            }

            break;
        case 3:
            if(algorithm_type.Get() == 0) {
                PrepareProjectorPatterns(&projector,
                                         config_file_calib_projector.Get(),
                                         &algo_gray_code_vert,
                                         config_file_structured_light_1.Get(),
                                         &algo_gray_code_horz,
                                         config_file_structured_light_2.Get(),
                                         true, // Firmware will NOT be uploaded
                                         &total_pattern_count);
            } else if(algorithm_type.Get() == 1) {
                PrepareProjectorPatterns(&projector,
                                         config_file_calib_projector.Get(),
                                         &algo_three_phase_vert,
                                         config_file_structured_light_1.Get(),
                                         &algo_three_phase_horz,
                                         config_file_structured_light_2.Get(),
                                         true, // Firmware will NOT be uploaded
                                         &total_pattern_count);

            } else {
                //  unreachable code
            }
            break;
        case 4:
            if(camera_type.Get() == 0) {
                CalibrateCamera(&camera_cv,
                                config_file_calib_camera.Get(),
                                calib_data_file_camera.Get(),
                                dir_camera_calib_image_output.Get() +
                                output_name_image_camera_calib.Get(),
                                &projector);
            } else if(camera_type.Get() == 1) {
                CalibrateCamera(&camera_pg,
                                config_file_calib_camera.Get(),
                                calib_data_file_camera.Get(),
                                dir_camera_calib_image_output.Get() +
                                output_name_image_camera_calib.Get(),
                                &projector);
            } else {
                //  unreachable code
            }
            break;
        case 5:
            if(camera_type.Get() == 0) {
                CalibrateSystem(&camera_cv,
                                config_file_calib_camera.Get(),
                                calib_data_file_camera.Get(),
                                &projector,
                                config_file_calib_projector.Get(),
                                calib_data_file_projector.Get(),
                                dir_system_calib_image_output.Get() +
                                output_name_image_system_calib.Get());
            } else if(camera_type.Get() == 1) {
                CalibrateSystem(&camera_pg,
                                config_file_calib_camera.Get(),
                                calib_data_file_camera.Get(),
                                &projector,
                                config_file_calib_projector.Get(),
                                calib_data_file_projector.Get(),
                                dir_system_calib_image_output.Get() +
                                output_name_image_system_calib.Get());
            } else {
                //  unreachable code
            }

            break;
        case 6:
            if(camera_type.Get() == 0) {
                if(algorithm_type.Get() == 0) {
                    ScanObject(&camera_cv,
                               false,
                               calib_data_file_camera.Get(),
                               &projector,
                               calib_data_file_projector.Get(),
                               &algo_gray_code_vert,
                               &algo_gray_code_horz,
                               true,
                               false,
                               config_file_geometry.Get(),
                               continuous_scanning.Get());
                } else if(algorithm_type.Get() == 1) {
                    ScanObject(&camera_cv,
                               false,
                               calib_data_file_camera.Get(),
                               &projector,
                               calib_data_file_projector.Get(),
                               &algo_three_phase_vert,
                               &algo_three_phase_horz,
                               true,
                               false,
                               config_file_geometry.Get(),
                               continuous_scanning.Get());
                } else {
                    //  unreachable code
                }
            } else if(camera_type.Get() == 1) {
                    if(algorithm_type.Get() == 0) {
                        ScanObject(&camera_pg,
                                   true,
                                   calib_data_file_camera.Get(),
                                   &projector,
                                   calib_data_file_projector.Get(),
                                   &algo_gray_code_vert,
                                   &algo_gray_code_horz,
                                   true,
                                   false,
                                   config_file_geometry.Get(),
                                   continuous_scanning.Get());
                    } else if(algorithm_type.Get() == 1) {
                        ScanObject(&camera_pg,
                                   true,
                                   calib_data_file_camera.Get(),
                                   &projector,
                                   calib_data_file_projector.Get(),
                                   &algo_three_phase_vert,
                                   &algo_three_phase_horz,
                                   true,
                                   false,
                                   config_file_geometry.Get(),
                                   continuous_scanning.Get());
                    } else {
                        //  unreachable code
                    }
            } else {
                //  unreachable code
            }
            break;
        case 7:
            if(camera_type.Get() == 0) {
                if(algorithm_type.Get() == 0) {
                    ScanObject(&camera_cv,
                               false,
                               calib_data_file_camera.Get(),
                               &projector,
                               calib_data_file_projector.Get(),
                               &algo_gray_code_vert,
                               &algo_gray_code_horz,
                               false,
                               true,
                               config_file_geometry.Get(),
                               continuous_scanning.Get());
                } else if(algorithm_type.Get() == 1) {
                    ScanObject(&camera_cv,
                               false,
                               calib_data_file_camera.Get(),
                               &projector,
                               calib_data_file_projector.Get(),
                               &algo_three_phase_vert,
                               &algo_three_phase_horz,
                               false,
                               true,
                               config_file_geometry.Get(),
                               continuous_scanning.Get());
                } else {
                    //  unreachable code
                }
            } else if(camera_type.Get() == 1) {
                if(algorithm_type.Get() == 0) {
                    ScanObject(&camera_pg,
                               true,
                               calib_data_file_camera.Get(),
                               &projector,
                               calib_data_file_projector.Get(),
                               &algo_gray_code_vert,
                               &algo_gray_code_horz,
                               false,
                               true,
                               config_file_geometry.Get(),
                               continuous_scanning.Get());
                } else if(algorithm_type.Get() == 1) {
                    ScanObject(&camera_pg,
                               true,
                               calib_data_file_camera.Get(),
                               &projector,
                               calib_data_file_projector.Get(),
                               &algo_three_phase_vert,
                               &algo_three_phase_horz,
                               false,
                               true,
                               config_file_geometry.Get(),
                               continuous_scanning.Get());
                } else {
                    //  unreachable code
                }
            } else {
                //  unreachable code
            }
            break;
        case 8:
            if(camera_type.Get() == 0) {
                if(algorithm_type.Get() == 0) {
					ScanObject(&camera_cv,
						false,
						calib_data_file_camera.Get(),
						&projector,
						calib_data_file_projector.Get(),
						&algo_gray_code_vert,
						&algo_gray_code_horz,
						true,
						true,
						config_file_geometry.Get(),
						continuous_scanning.Get(),
						8,
						3000
						);

//					_sleep(5 * 1000);					
                } else if(algorithm_type.Get() == 1) {
					ScanObject(&camera_cv,
                               false,
                               calib_data_file_camera.Get(),
                               &projector,
                               calib_data_file_projector.Get(),
                               &algo_three_phase_vert,
                               &algo_three_phase_horz,
                               true,
                               true,
                               config_file_geometry.Get(),
                               continuous_scanning.Get(),
							   8,
							   5000
							   );
//					_sleep(5 * 1000);
                } else {
                    //  unreachable code
                }
            } else if(camera_type.Get() == 1) {
                    if(algorithm_type.Get() == 0) {
                        ScanObject(&camera_pg,
                                   true,
                                   calib_data_file_camera.Get(),
                                   &projector,
                                   calib_data_file_projector.Get(),
                                   &algo_gray_code_vert,
                                   &algo_gray_code_horz,
                                   true,
                                   true,
                                   config_file_geometry.Get(),
                                   continuous_scanning.Get());
                    } else if(algorithm_type.Get() == 1) {
                        ScanObject(&camera_pg,
                                   true,
                                   calib_data_file_camera.Get(),
                                   &projector,
                                   calib_data_file_projector.Get(),
                                   &algo_three_phase_vert,
                                   &algo_three_phase_horz,
                                   true,
                                   true,
                                   config_file_geometry.Get(),
                                   continuous_scanning.Get());
                    } else {
                        //  unreachable code
                    }
            } else {
                //  unreachable code
            }
            break;
        case 9:
            // Disconnect system objects

            if(camera_type.Get() == 0) {
                camera_cv.Disconnect();
            } else if(camera_type.Get() == 1) {
                camera_pg.Disconnect();
            }
            projector.Disconnect();

            // Reconnect system objects
            dlp::DLP_Platform::ConnectSetup(projector,connect_id_projector.Get(),config_file_projector.Get(),true);

            if(camera_type.Get() == 0) {
                dlp::Camera::ConnectSetup(camera_cv,connect_id_camera.Get(),config_file_camera.Get(),true);
            } else if(camera_type.Get() == 1) {
                dlp::Camera::ConnectSetup(camera_pg,connect_id_camera.Get(),config_file_camera.Get(),true);
            } else {
                //  unreachable code
            }
            break;
        default:
            dlp::CmdLine::Print("Invalid menu selection! \n");
        }

        dlp::CmdLine::Print();
        dlp::CmdLine::Print();

    }while(menu_select != 0);

    // Disconnect system objects
    if(camera_type.Get() == 0) {
        camera_cv.Disconnect();
    } else if(camera_type.Get() == 1) {
        camera_pg.Disconnect();
    }

    projector.Disconnect();

    return 0;
}
