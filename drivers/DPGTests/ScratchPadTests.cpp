//
//  ScratchPadTests.cpp
//  Camellia
//
//  Created by Nathan Roberts on 4/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "ScratchPadTests.h"
#include "PenaltyConstraints.h"
#include "IP.h"
#include "PreviousSolutionFunction.h"
#include "RieszRep.h"

class UnitSquareBoundary : public SpatialFilter {
public:
  bool matchesPoint(double x, double y) {
    double tol = 1e-14;
    bool xMatch = (abs(x+1.0) < tol) || (abs(x-1.0) < tol);
    bool yMatch = (abs(y+1.0) < tol) || (abs(y-1.0) < tol);
//    cout << "UnitSquareBoundary: for (" << x << ", " << y << "), (xMatch, yMatch) = (" << xMatch << ", " << yMatch << ")\n";
    return xMatch || yMatch;
  }
};

class InflowSquareBoundary : public SpatialFilter {
public:
  bool matchesPoint(double x, double y) {
    double tol = 1e-14;
    bool xMatch = (abs(x) < tol);
    bool yMatch = (abs(y) < tol);
    return xMatch || yMatch;
  }
};

class Uinflow : public Function {
public:
  void values(FieldContainer<double> &values, BasisCachePtr basisCache){
    double tol = 1e-11;
    vector<int> cellIDs = basisCache->cellIDs();
    int numPoints = values.dimension(1);
    FieldContainer<double> points = basisCache->getPhysicalCubaturePoints();
    for (int i = 0;i<cellIDs.size();i++){
      for (int j = 0;j<numPoints;j++){
	double x = points(i,j,0);
	double y = points(i,j,1);
	values(i,j) = 0.0;
	if (abs(y)<tol){
	  values(i,j) = 1.0-x;
	}
	if (abs(x)<tol){
	  values(i,j) = 1.0-y;
	}

      }
    }
  }
};

// just for a discontinuity
class CellIDFunction : public Function {
public:
  void values(FieldContainer<double> &values, BasisCachePtr basisCache){
    vector<int> cellIDs = basisCache->cellIDs();
    int numPoints = values.dimension(1);
    FieldContainer<double> points = basisCache->getPhysicalCubaturePoints();
    for (int i = 0;i<cellIDs.size();i++){
      for (int j = 0;j<numPoints;j++){
	values(i,j) = cellIDs[i];
      }
    }
  }
};

// is zero except on the edge (.5, y) on a 2x1 unit quad mesh - an edge restriction function
class EdgeFunction : public Function {
public:
  bool boundaryValueOnly() { 
    return true; 
  } 
  void values(FieldContainer<double> &values, BasisCachePtr basisCache){
    double tol = 1e-11;
    vector<int> cellIDs = basisCache->cellIDs();
    int numPoints = values.dimension(1);
    FieldContainer<double> points = basisCache->getPhysicalCubaturePoints();
    for (int i = 0;i<cellIDs.size();i++){
      for (int j = 0;j<numPoints;j++){
	double x = points(i,j,0);
	double y = points(i,j,1);
	if (abs(x-.5)<tol){
	  values(i,j) = 1.0;
	}else{
	  values(i,j) = 0.0;
	}
      }
    }
  }
};


class PositiveX : public SpatialFilter {
public:
  bool matchesPoint(double x, double y) {
    return x > 0;
  }
};

void ScratchPadTests::setup() {
  ////////////////////   DECLARE VARIABLES   ///////////////////////
  // define test variables
  VarFactory varFactory; 
  VarPtr tau = varFactory.testVar("\\tau", HDIV);
  VarPtr v = varFactory.testVar("v", HGRAD);
  
  // define trial variables
  VarPtr uhat = varFactory.traceVar("\\widehat{u}");
  VarPtr beta_n_u_minus_sigma_n = varFactory.fluxVar("\\widehat{\\beta \\cdot n u - \\sigma_{n}}");
  VarPtr u = varFactory.fieldVar("u");
  VarPtr sigma1 = varFactory.fieldVar("\\sigma_1");
  VarPtr sigma2 = varFactory.fieldVar("\\sigma_2");
  
  vector<double> beta_const;
  beta_const.push_back(2.0);
  beta_const.push_back(1.0);
  
  double eps = 1e-2;
  
  // standard confusion bilinear form
  _confusionBF = Teuchos::rcp( new BF(varFactory) );
  // tau terms:
  _confusionBF->addTerm(sigma1 / eps, tau->x());
  _confusionBF->addTerm(sigma2 / eps, tau->y());
  _confusionBF->addTerm(u, tau->div());
  _confusionBF->addTerm(-uhat, tau->dot_normal());
  
  // v terms:
  _confusionBF->addTerm( sigma1, v->dx() );
  _confusionBF->addTerm( sigma2, v->dy() );
  _confusionBF->addTerm( beta_const * u, - v->grad() );
  _confusionBF->addTerm( beta_n_u_minus_sigma_n, v);

  _uhat_confusion = uhat; // confusion variable u_hat
  
  ////////////////////   BUILD MESH   ///////////////////////
  // define nodes for mesh
  FieldContainer<double> quadPoints(4,2);
  
  quadPoints(0,0) = -1.0; // x1
  quadPoints(0,1) = -1.0; // y1
  quadPoints(1,0) = 1.0;
  quadPoints(1,1) = -1.0;
  quadPoints(2,0) = 1.0;
  quadPoints(2,1) = 1.0;
  quadPoints(3,0) = -1.0;
  quadPoints(3,1) = 1.0;
  
  int H1Order = 1, pToAdd = 0;
  int horizontalCells = 1, verticalCells = 1;
  
  // create a pointer to a new mesh:
  _spectralConfusionMesh = Mesh::buildQuadMesh(quadPoints, horizontalCells, verticalCells,
                                               _confusionBF, H1Order, H1Order+pToAdd);
  
  // some 2D test points:
  // setup test points:
  static const int NUM_POINTS_1D = 10;
  double x[NUM_POINTS_1D] = {-1.0,-0.8,-0.6,-.4,-.2,0,0.2,0.4,0.6,0.8};
  double y[NUM_POINTS_1D] = {-0.8,-0.6,-.4,-.2,0,0.2,0.4,0.6,0.8,1.0};
  
  _testPoints = FieldContainer<double>(NUM_POINTS_1D*NUM_POINTS_1D,2);
  for (int i=0; i<NUM_POINTS_1D; i++) {
    for (int j=0; j<NUM_POINTS_1D; j++) {
      _testPoints(i*NUM_POINTS_1D + j, 0) = x[i];
      _testPoints(i*NUM_POINTS_1D + j, 1) = y[i];
    }
  }
  
  _elemType = _spectralConfusionMesh->getElement(0)->elementType();
  vector<int> cellIDs;
  int cellID = 0;
  cellIDs.push_back(cellID);
  _basisCache = Teuchos::rcp( new BasisCache( _elemType, _spectralConfusionMesh ) );
  _basisCache->setRefCellPoints(_testPoints);
  
  _basisCache->setPhysicalCellNodes( _spectralConfusionMesh->physicalCellNodesForCell(cellID), cellIDs, true );

}

void ScratchPadTests::teardown() {
  
}

void ScratchPadTests::runTests(int &numTestsRun, int &numTestsPassed) {
  setup();
  if (testPenaltyConstraints()) {
    numTestsPassed++;
  }
  numTestsRun++;
  teardown();
  
  setup();
  if (testSpatiallyFilteredFunction()) {
    numTestsPassed++;
  }
  numTestsRun++;
  teardown();
  
  setup();
  if (testConstantFunctionProduct()) {
    numTestsPassed++;
  }
  numTestsRun++;
  teardown();
  
  setup();
  if (testLinearTermEvaluationConsistency()) {
    numTestsPassed++;
  }
  numTestsRun++;
  teardown();   
  
  setup();
  if (testErrorOrthogonality()) {
    numTestsPassed++;
  }
  numTestsRun++;
  teardown();   
  setup();

  if (testIntegrateDiscontinuousFunction()) {
    numTestsPassed++;
  }
  numTestsRun++;
  teardown();   
}

bool ScratchPadTests::testConstantFunctionProduct() {
  bool success = true;
  // set up basisCache (even though it won't really be used here)
  BasisCachePtr basisCache = Teuchos::rcp( new BasisCache( _elemType, _spectralConfusionMesh ) );
  vector<int> cellIDs;
  int cellID = 0;
  cellIDs.push_back(cellID);
  basisCache->setPhysicalCellNodes( _spectralConfusionMesh->physicalCellNodesForCell(cellID), 
                                   cellIDs, true );
  
  int numCells = _basisCache->getPhysicalCubaturePoints().dimension(0);
  int numPoints = _testPoints.dimension(0);
  FunctionPtr three = Teuchos::rcp( new ConstantScalarFunction(3.0) );
  FunctionPtr two = Teuchos::rcp( new ConstantScalarFunction(2.0) );

  FieldContainer<double> values(numCells,numPoints);
  two->values(values,basisCache);
  three->scalarMultiplyBasisValues( values, basisCache );
  
  FieldContainer<double> expectedValues(numCells,numPoints);
  expectedValues.initialize( 3.0 * 2.0 );
  
  double tol = 1e-15;
  double maxDiff = 0.0;
  if ( ! fcsAgree(expectedValues, values, tol, maxDiff) ) {
    success = false;
    cout << "Expected product differs from actual; maxDiff: " << maxDiff << endl;
  }
  return success;
}

bool ScratchPadTests::testPenaltyConstraints() {
  bool success = true;
  int numCells = 1;
  FunctionPtr one = Teuchos::rcp( new ConstantScalarFunction(1.0) );
  
  SpatialFilterPtr entireBoundary = Teuchos::rcp( new UnitSquareBoundary );
  
  Teuchos::RCP<PenaltyConstraints> pc = Teuchos::rcp(new PenaltyConstraints);
  pc->addConstraint(_uhat_confusion==one,entireBoundary);
  
  FieldContainer<double> localRHSVector(numCells,_elemType->trialOrderPtr->totalDofs());
  FieldContainer<double> localStiffness(numCells,_elemType->trialOrderPtr->totalDofs(),
                                        _elemType->trialOrderPtr->totalDofs());
  
  // Our basis for uhat is 1-x, 1+x -- we should figure out what that means for
  // the values of the integrals that go into expectedStiffness.  For now, focus
  // on the sparsity pattern.
  
  int trialDofs = _elemType->trialOrderPtr->totalDofs();
  FieldContainer<double> expectedSparsity(numCells,_elemType->trialOrderPtr->totalDofs(),
                                          _elemType->trialOrderPtr->totalDofs());
  FieldContainer<double> expectedRHSSparsity(numCells,_elemType->trialOrderPtr->totalDofs());
  
  for (int cellIndex=0; cellIndex<numCells; cellIndex++) {
    for (int sideIndex=0; sideIndex<4; sideIndex++) {
      vector<int> uhat_dofIndices = _elemType->trialOrderPtr->getDofIndices(_uhat_confusion->ID(),sideIndex);
    
      for (int dofOrdinal1=0; dofOrdinal1 < uhat_dofIndices.size(); dofOrdinal1++) {
        int dofIndex1 = uhat_dofIndices[dofOrdinal1];
        expectedRHSSparsity(cellIndex,dofIndex1) = 1.0;
        for (int dofOrdinal2=0; dofOrdinal2 < uhat_dofIndices.size(); dofOrdinal2++) {
          int dofIndex2 = uhat_dofIndices[dofOrdinal2];
          expectedSparsity(cellIndex,dofIndex1,dofIndex2) = 1.0;
        }
      }
    }
  }
  
  Teuchos::RCP<BCEasy> bc = Teuchos::rcp( new BCEasy );

  pc->filter(localStiffness, localRHSVector, _basisCache, _spectralConfusionMesh, bc);
  
//  cout << "testPenaltyConstraints: expectedStiffnessSparsity:\n" << expectedSparsity;
//  cout << "testPenaltyConstraints: localStiffness:\n" << localStiffness;
//  
//  cout << "testPenaltyConstraints: expectedRHSSparsity:\n" << expectedRHSSparsity;
//  cout << "testPenaltyConstraints: localRHSVector:\n" << localRHSVector;
  
  // compare sparsity
  for (int cellIndex=0; cellIndex<numCells; cellIndex++) {
    for (int i=0; i<trialDofs; i++) {
      double rhsValue = localRHSVector(cellIndex,i);
      double rhsSparsityValue = expectedRHSSparsity(cellIndex,i);
      if ((rhsSparsityValue == 0.0) && (rhsValue != 0.0)) {
        cout << "testPenaltyConstraints rhs: expected 0 but found " << rhsValue << " at i = " << i << ".\n";
        success = false;
      }
      if ((rhsSparsityValue != 0.0) && (rhsValue == 0.0)) {
        cout << "testPenaltyConstraints rhs: expected nonzero but found 0 at i = " << i << ".\n";
        success = false;
      }
      for (int j=0; j<trialDofs; j++) {
        double stiffValue = localStiffness(cellIndex,i,j);
        double sparsityValue = expectedSparsity(cellIndex,i,j);
        if ((sparsityValue == 0.0) && (stiffValue != 0.0)) {
          cout << "testPenaltyConstraints stiffness: expected 0 but found " << stiffValue << " at (" << i << ", " << j << ").\n";
          success = false;
        }
        if ((sparsityValue != 0.0) && (stiffValue == 0.0)) {
          cout << "testPenaltyConstraints stiffness: expected nonzero but found 0 at (" << i << ", " << j << ").\n";
          success = false;
        }
      }
    }
  }
  return success;
}

bool ScratchPadTests::testSpatiallyFilteredFunction() {
  bool success = true;
  FunctionPtr one = Teuchos::rcp( new ConstantScalarFunction(1.0) );
  SpatialFilterPtr positiveX = Teuchos::rcp( new PositiveX );
  FunctionPtr heaviside = Teuchos::rcp( new SpatiallyFilteredFunction(one, positiveX) );
  
  int numCells = _basisCache->getPhysicalCubaturePoints().dimension(0);
  int numPoints = _testPoints.dimension(0);
  
  FieldContainer<double> values(numCells,numPoints);
  FieldContainer<double> expectedValues(numCells,numPoints);
  
  for (int cellIndex=0; cellIndex<numCells; cellIndex++) {
    for (int ptIndex=0; ptIndex<numPoints;ptIndex++) {
      double x = _basisCache->getPhysicalCubaturePoints()(cellIndex,ptIndex,0);
      if (x > 0) {
        expectedValues(cellIndex,ptIndex) = 1.0;
      } else {
        expectedValues(cellIndex,ptIndex) = 0.0;
      }
    }
  }
  
  heaviside->values(values,_basisCache);
  
  double tol = 1e-15;
  double maxDiff = 0.0;
  if ( ! fcsAgree(expectedValues, values, tol, maxDiff) ) {
    success = false;
    cout << "testSpatiallyFilteredFunction: Expected values differ from actual; maxDiff: " << maxDiff << endl;
  }
  return success;
}

// tests whether a mixed type LT
bool ScratchPadTests::testLinearTermEvaluationConsistency(){
  bool success = true;

  ////////////////////   DECLARE VARIABLES   ///////////////////////
  // define test variables
  VarFactory varFactory; 
  VarPtr v = varFactory.testVar("v", HGRAD);
  
  vector<double> beta;
  beta.push_back(1.0);
  beta.push_back(1.0);
  
  ////////////////////   DEFINE INNER PRODUCT(S)   ///////////////////////

   // robust test norm
  IPPtr ip = Teuchos::rcp(new IP);
  ip->addTerm(v);
  ip->addTerm(beta*v->grad());

  // define trial variables
  VarPtr beta_n_u = varFactory.fluxVar("\\widehat{\\beta \\cdot n }");
  VarPtr u = varFactory.fieldVar("u");

  ////////////////////   BUILD MESH   ///////////////////////

  BFPtr convectionBF = Teuchos::rcp( new BF(varFactory) );
  
  // v terms:
  convectionBF->addTerm( -u, beta * v->grad() );
  convectionBF->addTerm( beta_n_u, v);

  // define nodes for mesh
  int order = 1;
  int H1Order = order+1; int pToAdd = 1;
  
  // create a pointer to a new mesh:
  Teuchos::RCP<Mesh> mesh = Mesh::buildUnitQuadMesh(1, convectionBF, H1Order, H1Order+pToAdd);

  
  ////////////////////   get fake residual   ///////////////////////

  LinearTermPtr lt = Teuchos::rcp(new LinearTerm);
  FunctionPtr edgeFxn = Teuchos::rcp(new EdgeFunction);
  FunctionPtr Xsq = Teuchos::rcp(new Xn(2));
  FunctionPtr Ysq = Teuchos::rcp(new Yn(2));
  FunctionPtr XYsq = Xsq*Ysq;
  lt->addTerm(edgeFxn*v + (beta*XYsq)*v->grad());

  Teuchos::RCP<RieszRep> ltRiesz = Teuchos::rcp(new RieszRep(mesh, ip, lt));
  ltRiesz->computeRieszRep();
  FunctionPtr repFxn = Teuchos::rcp(new RepFunction(v,ltRiesz));
  map<int,FunctionPtr> rep_map;
  rep_map[v->ID()] = repFxn;

  FunctionPtr edgeLt = lt->evaluate(rep_map, true) ;
  FunctionPtr elemLt = lt->evaluate(rep_map, false);

  double edgeVal = edgeLt->integrate(mesh,10);
  double elemVal = elemLt->integrate(mesh,10);
  LinearTermPtr edgeOnlyLt = Teuchos::rcp(new LinearTerm);// residual 
  edgeOnlyLt->addTerm(edgeFxn*v);
  FunctionPtr edgeOnly = edgeOnlyLt->evaluate(rep_map,true);
  double edgeOnlyVal = edgeOnly->integrate(mesh,10);

  double diff = edgeOnlyVal-edgeVal;
  if (abs(diff)>1e-11){
    success = false;
    cout << "Failed testLinearTermEvaluationConsistency() with diff = " << diff << endl;
  }
  
  return success;
}

// tests whether a mixed type LT
bool ScratchPadTests::testIntegrateDiscontinuousFunction(){
  bool success = true;

  ////////////////////   DECLARE VARIABLES   ///////////////////////
  // define test variables
  VarFactory varFactory; 
  VarPtr v = varFactory.testVar("v", HGRAD);
  
  vector<double> beta;
  beta.push_back(1.0);
  beta.push_back(1.0);
  
  ////////////////////   DEFINE INNER PRODUCT(S)   ///////////////////////

   // robust test norm
  IPPtr ip = Teuchos::rcp(new IP);
  ip->addTerm(v);
  ip->addTerm(beta*v->grad());

  // define trial variables
  VarPtr beta_n_u = varFactory.fluxVar("\\widehat{\\beta \\cdot n }");
  VarPtr u = varFactory.fieldVar("u");

  ////////////////////   BUILD MESH   ///////////////////////

  BFPtr convectionBF = Teuchos::rcp( new BF(varFactory) );
  
  // v terms:
  convectionBF->addTerm( -u, beta * v->grad() );
  convectionBF->addTerm( beta_n_u, v);

  // define nodes for mesh
  int order = 1;
  int H1Order = order+1; int pToAdd = 1;
  
  // create a pointer to a new mesh:
  Teuchos::RCP<Mesh> mesh = Mesh::buildUnitQuadMesh(2, 1, convectionBF, H1Order, H1Order+pToAdd);
  
  ////////////////////   integrate discontinuous function - cellIDFunction   ///////////////////////

  FunctionPtr cellIDFxn = Teuchos::rcp(new CellIDFunction); // should be 0 on cellID 0, 1 on cellID 1
  double jumpWeight = 13.3; // some random number
  FunctionPtr edgeRestrictionFxn = Teuchos::rcp(new EdgeFunction);
  FunctionPtr X = Teuchos::rcp(new Xn(1));
  LinearTermPtr integrandLT = Function::constant(1.0)*v + Function::constant(jumpWeight)*X*edgeRestrictionFxn*v;
  
  map<int,FunctionPtr> vmap;
  vmap[v->ID()] = cellIDFxn;

  FunctionPtr volumeIntegrand = integrandLT->evaluate(vmap,false);
  FunctionPtr edgeIntegrand = integrandLT->evaluate(vmap,true);
  double value = volumeIntegrand->integrate(mesh,10) + edgeIntegrand->integrate(mesh,10);
  double expectedValue = .5*(jumpWeight+1.0);
  double diff = abs(expectedValue-value);
  if (abs(diff)>1e-11){
    success = false;
    cout << "Failed testIntegrateDiscontinuousFunction() with expectedValue = " << expectedValue << " and actual value = " << value << endl;
  }  
  return success;
}

// tests whether a mixed type LT
bool ScratchPadTests::testErrorOrthogonality(){
  bool success = true;

  ////////////////////   DECLARE VARIABLES   ///////////////////////
  // define test variables
  VarFactory varFactory; 
  VarPtr v = varFactory.testVar("v", HGRAD);
  
  vector<double> beta;
  beta.push_back(1.0);
  beta.push_back(1.0);
  
  ////////////////////   DEFINE INNER PRODUCT(S)   ///////////////////////

   // robust test norm
  IPPtr ip = Teuchos::rcp(new IP);
  ip->addTerm(v);
  ip->addTerm(beta*v->grad());

  // define trial variables
  VarPtr beta_n_u = varFactory.fluxVar("\\widehat{\\beta \\cdot n }");
  VarPtr u = varFactory.fieldVar("u");

  ////////////////////   BUILD MESH   ///////////////////////

  BFPtr convectionBF = Teuchos::rcp( new BF(varFactory) );
  
  // v terms:
  convectionBF->addTerm( -u, beta * v->grad() );
  convectionBF->addTerm( beta_n_u, v);

  // define nodes for mesh
  int order = 1;
  int H1Order = order+1; int pToAdd = 1;
  
  // create a pointer to a new mesh:
  Teuchos::RCP<Mesh> mesh = Mesh::buildUnitQuadMesh(1, convectionBF, H1Order, H1Order+pToAdd);
  
  ////////////////////   SOLVE & REFINE   ///////////////////////

  Teuchos::RCP<RHSEasy> rhs = Teuchos::rcp( new RHSEasy );
  Teuchos::RCP<BCEasy> bc = Teuchos::rcp( new BCEasy );
  SpatialFilterPtr inflowBoundary = Teuchos::rcp( new InflowSquareBoundary );
  SpatialFilterPtr outflowBoundary = Teuchos::rcp( new NegatedSpatialFilter(inflowBoundary) );
  
  FunctionPtr uIn = Teuchos::rcp(new Uinflow);
  FunctionPtr n = Teuchos::rcp(new UnitNormalFunction);
  bc->addDirichlet(beta_n_u, inflowBoundary, beta*n*uIn);  

  Teuchos::RCP<Solution> solution;
  solution = Teuchos::rcp( new Solution(mesh, bc, rhs, ip) );  
  solution->solve(false);
  FunctionPtr uCopy = Teuchos::rcp( new PreviousSolutionFunction(solution, u) );
  FunctionPtr fnhatCopy = Teuchos::rcp( new PreviousSolutionFunction(solution, beta_n_u));
  
  ////////////////////   get residual   ///////////////////////

  LinearTermPtr residual = Teuchos::rcp(new LinearTerm);// residual 
  residual->addTerm(-fnhatCopy*v + (beta*uCopy)*v->grad());

  Teuchos::RCP<RieszRep> riesz = Teuchos::rcp(new RieszRep(mesh, ip, residual));
  riesz->computeRieszRep();
  double rieszErr = riesz->getNorm();
  double energyErr = solution->energyErrorTotal();

  // do initial check on consistency
  if (abs(rieszErr-energyErr)>1e-11){
    success = false;
    cout << "Failed testErrorOrthogonality() because riesz and energy error are inconsistent with each other - the energy error using the RieszRep class = " << rieszErr << ", while the energy error computed in Solution.cpp is " << energyErr << endl;
    return success;
  }
  FunctionPtr rieszRepFxn = Teuchos::rcp(new RepFunction(v,riesz));

  map<int,FunctionPtr> err_rep_map;
  err_rep_map[v->ID()] = rieszRepFxn;
  LinearTermPtr edgeOrthogonalityCheckLT = Teuchos::rcp(new LinearTerm);// residual 
  LinearTermPtr elemOrthogonalityCheckLT = Teuchos::rcp(new LinearTerm);// residual 
  FunctionPtr fnhatOutflow = Teuchos::rcp(new SpatiallyFilteredFunction(fnhatCopy,outflowBoundary));
  edgeOrthogonalityCheckLT->addTerm(-fnhatOutflow*v,true);
  elemOrthogonalityCheckLT->addTerm((beta*uCopy)*v->grad(),true);

  FunctionPtr edgeResidual = edgeOrthogonalityCheckLT->evaluate(err_rep_map, true) ;
  FunctionPtr elemResidual = elemOrthogonalityCheckLT->evaluate(err_rep_map, false);

  double edgeVal = edgeResidual->integrate(mesh,10);
  double elemVal = elemResidual->integrate(mesh,10);
  
  double fieldDiff = elemVal;
  double fluxDiff = edgeVal;
  double diff = abs(fluxDiff)+abs(fieldDiff);
  if (abs(diff)>1e-11){
    success = false;
    cout << "fnhatCopy boundary part only evaluates to = " << fnhatCopy->boundaryValueOnly() << endl;
    cout << "Failed testErrorOrthogonality() with field diff = " << fieldDiff << " and flux diff = " << fluxDiff << endl;
  }
  
  return success;
}

std::string ScratchPadTests::testSuiteName() {
  return "ScratchPadTests";
}
