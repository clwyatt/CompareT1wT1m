/*****************************************************************************
Copyright (c) 2011, Chris Wyatt, Bioimaging Systems Lab, Virginia Tech
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
#include <itkMinimumMaximumImageFilter.h>

#include <gdcmGlobal.h>
#include <gdcmDictSet.h>

// command line parsing
#include "vul_arg.h"

typedef short PixelType;
typedef itk::OrientedImage<PixelType, 3> Image3DType;
typedef itk::OrientedImage<PixelType, 2> Image2DType;
typedef itk::OrientedImage<unsigned char, 2> ImageRenderType;

Image3DType::Pointer ReadDICOM( std::string dir )
{
  // create name generator and attach to reader
  itk::GDCMSeriesFileNames::Pointer nameGenerator = itk::GDCMSeriesFileNames::New();
  nameGenerator->SetUseSeriesDetails(true);
  nameGenerator->AddSeriesRestriction("0020|0100"); // acquisition number
  nameGenerator->SetDirectory( dir.c_str() );

  // get series IDs, use the first encountered
  const std::vector<std::string>& seriesUID = nameGenerator->GetSeriesUIDs();
  // if( seriesUID.size() == 0)
  //   {
  //   cerr << "Error: no DICOM series found in " << dir.c_str() << endl;
  //   }
  std::vector<std::string>::const_iterator seriesItr=seriesUID.begin();
  // if( seriesUID.size() != 1)
  //   {
  //   clog << "Warning: multiple series found, using the first (UID "
  // 	 << seriesItr->c_str() << ")" << endl;
  //   }

  std::vector<std::string> fileNames;

  itk::ImageSeriesReader<Image3DType>::Pointer tmp_reader =
    itk::ImageSeriesReader<Image3DType>::New();

  itk::GDCMImageIO::Pointer dicomIO = itk::GDCMImageIO::New();
  dicomIO->KeepOriginalUIDOn();
  tmp_reader->SetImageIO(dicomIO);

  fileNames = nameGenerator->GetFileNames(seriesItr->c_str());
  tmp_reader->SetFileNames(fileNames);
  tmp_reader->Update();

  return tmp_reader->GetOutput();
}

Image2DType::Pointer ExtractSliceFromVolume( Image3DType::Pointer image, unsigned int axis )
{

  typedef itk::ExtractImageFilter< Image3DType, Image2DType> FilterType;
  FilterType::Pointer filter = FilterType::New();

  Image3DType::RegionType inputRegion =
    image->GetLargestPossibleRegion();

  Image3DType::SizeType size = inputRegion.GetSize();
  Image3DType::IndexType start = inputRegion.GetIndex();
  const unsigned int sliceNumber = size[axis] >> 1;
  size[axis] = 0;
  start[axis] = sliceNumber;

  Image3DType::RegionType desiredRegion;
  desiredRegion.SetSize( size );
  desiredRegion.SetIndex( start );

  filter->SetExtractionRegion( desiredRegion );

  filter->SetInput( image );
  filter->Update();

  return filter->GetOutput();
}

ImageRenderType::Pointer RenderImage( Image2DType::Pointer image )
{
  typedef itk::MinimumMaximumImageFilter< Image2DType > MinMaxFilterType;
  MinMaxFilterType::Pointer minmaxfilter = MinMaxFilterType::New();
  minmaxfilter->SetInput( image );
  minmaxfilter->Update();

  typedef itk::IntensityWindowingImageFilter< Image2DType, ImageRenderType> WindowLevelFilterType;
  WindowLevelFilterType::Pointer wlfilter =  WindowLevelFilterType::New();
  wlfilter->SetInput( image );
  wlfilter->SetWindowMinimum( minmaxfilter->GetMinimum() );
  wlfilter->SetWindowMaximum( minmaxfilter->GetMaximum() );
  wlfilter->SetOutputMinimum(0);
  wlfilter->SetOutputMaximum(255);

  wlfilter->Update();

  return wlfilter->GetOutput();
}

void LoadExtractWrite( std::string indir, std::string outfile )
{

  Image3DType::Pointer dcmT1w = ReadDICOM( indir );

  Image2DType::Pointer slice = ExtractSliceFromVolume( dcmT1w, 1 );

  ImageRenderType::Pointer renderedImage = RenderImage( slice );

  typedef itk::ImageFileWriter< ImageRenderType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput( renderedImage );
  writer->SetFileName( outfile );
  writer->Update();
}

int main(int argc, char** argv)
{
  // command line args
  vul_arg<std::string> dirT1w(0, "T1w Input DICOM DIR");
  vul_arg<std::string> dirT1m(0, "T1m Input DICOM DIR");
  vul_arg<std::string> outfilebase(0, "Output File Base");
  vul_arg_parse(argc, argv);

  std::string outfileT1w = outfilebase() + "_T1w.png";

  std::string outfileT1m = outfilebase() + "_T1m.png";

  LoadExtractWrite( dirT1w(), outfileT1w );

  LoadExtractWrite( dirT1m(), outfileT1m );

  return EXIT_SUCCESS;
}
