/*****************************************************************************
Copyright (c) 2010, Bioimaging Systems Lab, Virginia Tech
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of Virgina Tech nor the names of its contributors may
   be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*******************************************************************************/
#include <iostream>
using std::cout;
using std::cerr;
using std::clog;
using std::endl;

#include <string>
#include <cassert>

#include <itkMetaDataDictionary.h>
#include <itkImageSeriesReader.h>
#include <itkImageSeriesWriter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkOrientedImage.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkNumericSeriesFileNames.h>
#include <itkExtractImageFilter.h>
#include <itkIntensityWindowingImageFilter.h>

#include <gdcmGlobal.h>
#include <gdcmDictSet.h>

// command line parsing
#include "vul_arg.h"

typedef short PixelType;
typedef itk::OrientedImage<PixelType, 3> Image3DType;
typedef itk::OrientedImage<PixelType, 2> Image2DType;
typedef itk::OrientedImage<unsigned char, 2> ImageRenderType;

int main(int argc, char** argv)
{
  // command line args
  vul_arg<std::string> dirT1w(0, "T1w Input DICOM DIR");
  vul_arg<std::string> dirT1m(0, "T1m Input DICOM DIR");
  vul_arg<std::string> outfile(0, "Output File");
  vul_arg_parse(argc, argv);

  cout << "---Reading T1w DICOM---" << endl;
  // create name generator and attach to reader
  itk::GDCMSeriesFileNames::Pointer nameGenerator = itk::GDCMSeriesFileNames::New();
  nameGenerator->SetUseSeriesDetails(true);
  nameGenerator->AddSeriesRestriction("0020|0100"); // acquisition number
  nameGenerator->SetDirectory( dirT1w().c_str() );

  // get series IDs, use the first encountered
  const std::vector<std::string>& seriesUID = nameGenerator->GetSeriesUIDs();
  if( seriesUID.size() == 0)
    {
    cerr << "Error: no DICOM series found in " << dirT1w() << endl;
    }
  std::vector<std::string>::const_iterator seriesItr=seriesUID.begin();
  if( seriesUID.size() != 1)
    {
    clog << "Warning: multiple series found, using the first (UID "
	 << seriesItr->c_str() << ")" << endl;
    }

  std::vector<std::string> fileNames;

  itk::ImageSeriesReader<Image3DType>::Pointer tmp_reader =
    itk::ImageSeriesReader<Image3DType>::New();

  itk::GDCMImageIO::Pointer dicomIO = itk::GDCMImageIO::New();
  dicomIO->KeepOriginalUIDOn();
  tmp_reader->SetImageIO(dicomIO);

  fileNames = nameGenerator->GetFileNames(seriesItr->c_str());
  tmp_reader->SetFileNames(fileNames);
  tmp_reader->Update();

  tmp_reader->GetOutput()->SetMetaDataDictionary
    (dicomIO->GetMetaDataDictionary());

  Image3DType::Pointer dcmT1w = tmp_reader->GetOutput();

  typedef itk::ExtractImageFilter< Image3DType, Image2DType> FilterType;
  FilterType::Pointer filter = FilterType::New();

  Image3DType::RegionType inputRegion =
    dcmT1w->GetLargestPossibleRegion();

  Image3DType::SizeType size = inputRegion.GetSize();
  Image3DType::IndexType start = inputRegion.GetIndex();
  const unsigned int sliceNumber = size[2] >> 1;
  size[2] = 0;
  start[2] = sliceNumber;

  cout << "Slice Number " << sliceNumber << endl;

  Image3DType::RegionType desiredRegion;
  desiredRegion.SetSize( size );
  desiredRegion.SetIndex( start );

  filter->SetExtractionRegion( desiredRegion );

  filter->SetInput( dcmT1w );


  typedef itk::IntensityWindowingImageFilter< Image2DType, ImageRenderType> WindowLevelFilterType;
  WindowLevelFilterType::Pointer wlfilter =  WindowLevelFilterType::New();
  wlfilter->SetInput( filter->GetOutput() );
  wlfilter->SetWindowMinimum(0);
  wlfilter->SetWindowMaximum(32000);
  wlfilter->SetOutputMinimum(0);
  wlfilter->SetOutputMaximum(255);

  typedef itk::ImageFileWriter< ImageRenderType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput( wlfilter->GetOutput() );
  writer->SetFileName( outfile() );

  writer->Update();

  // cout << "---Reading T1m DICOM---" << endl;
  // // create name generator and attach to reader
  // itk::GDCMSeriesFileNames::Pointer nameGenerator2 = itk::GDCMSeriesFileNames::New();
  // nameGenerator2->SetUseSeriesDetails(true);
  // nameGenerator2->AddSeriesRestriction("0020|0100"); // acquisition number
  // nameGenerator2->SetDirectory( dirT1m().c_str() );

  // // get series IDs, use the first encountered
  // const std::vector<std::string>& seriesUID2 = nameGenerator2->GetSeriesUIDs();
  // if( seriesUID2.size() == 0)
  //   {
  //   cerr << "Error: no DICOM series found in " << dirT1m() << endl;
  //   }
  // std::vector<std::string>::const_iterator seriesItr2=seriesUID2.begin();
  // if( seriesUID2.size() != 1)
  //   {
  //   clog << "Warning: multiple series found, using the first (UID "
  // 	 << seriesItr2->c_str() << ")" << endl;
  //   }

  // std::string value;
  // std::vector<std::string> fileNames2;

  // itk::ImageSeriesReader<Image3DType>::Pointer tmp_reader2 =
  //   itk::ImageSeriesReader<Image3DType>::New();

  // itk::GDCMImageIO::Pointer dicomIO2 = itk::GDCMImageIO::New();
  // dicomIO2->KeepOriginalUIDOn();
  // tmp_reader2->SetImageIO(dicomIO);

  // fileNames2 = nameGenerator->GetFileNames(seriesItr->c_str());
  // tmp_reader2->SetFileNames(fileNames2);
  // tmp_reader2->Update();

  // tmp_reader2->GetOutput()->SetMetaDataDictionary
  //   (dicomIO2->GetMetaDataDictionary());

  // Image3DType::Pointer dcmT1m = tmp_reader2->GetOutput();

  // /* Find the window center */
  // value.clear();
  // dicomIO2->GetValueFromTag("0028|1050", value);
  // std::cout << "T1m window center: " << atof(value.c_str()) << std::endl;


  return EXIT_SUCCESS;
}
