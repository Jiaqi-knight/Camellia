#ifndef TEST_BILINEAR_FORM_FLUX
#define TEST_BILINEAR_FORM_FLUX

// @HEADER
//
// Copyright © 2011 Sandia Corporation. All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are 
// permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of 
// conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of 
// conditions and the following disclaimer in the documentation and/or other materials 
// provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived from 
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY 
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR 
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Nate Roberts (nate@nateroberts.com).
//
// @HEADER 

#include "BilinearForm.h"

/*
 This test bilinear form just has b(u,v) = Int_dK (u_hat, v ),
 where u_hat is a trace (belongs formally to H^(1/2)), and \tau is a test
 function, belonging to H(div,K).
 */

class TestBilinearFormFlux : public BilinearForm {
private:
//  static const string & S_TEST;
//  static const string & S_TRIAL;
  
public:
  TestBilinearFormFlux() {
    _testIDs.push_back(0);
    _trialIDs.push_back(0);
  }
  
  // implement the virtual methods declared in super:
  const string & testName(int testID) {
    const static string S_TEST = "test";
    return S_TEST;
  }
  const string & trialName(int trialID) {
    const static string S_TRIAL = "flux";
    return S_TRIAL;
  }
  
  bool trialTestOperator(int trialID, int testID, 
                         EOperatorExtended &trialOperator, EOperatorExtended &testOperator) {
    trialOperator = IntrepidExtendedTypes::OPERATOR_VALUE;
    testOperator  = IntrepidExtendedTypes::OPERATOR_VALUE;
    return true;
  }
  
  void applyBilinearFormData(int trialID, int testID,
                           FieldContainer<double> &trialValues, FieldContainer<double> &testValues,
                           FieldContainer<double> &points) {
    // leave values as they are...             
  }
  
  EFunctionSpaceExtended functionSpaceForTest(int testID) {
    return IntrepidExtendedTypes::FUNCTION_SPACE_HGRAD;
  }
  
  EFunctionSpaceExtended functionSpaceForTrial(int trialID) {
    return IntrepidExtendedTypes::FUNCTION_SPACE_HVOL;
  }
  
  bool isFluxOrTrace(int trialID) {
    return true;
  }
  
};

#endif
