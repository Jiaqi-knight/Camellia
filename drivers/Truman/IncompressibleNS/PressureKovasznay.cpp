//  Camellia
//
//  Created by Truman Ellis on 6/4/2012.

#include "InnerProductScratchPad.h"
#include "PreviousSolutionFunction.h"
#include "RefinementStrategy.h"
#include "RefinementHistory.h"
#include "SolutionExporter.h"
#include "MeshFactory.h"
#include "CheckConservation.h"
#include "LagrangeConstraints.h"
#include "PenaltyConstraints.h"

#ifdef HAVE_MPI
#include <Teuchos_GlobalMPISession.hpp>
#include "mpi_choice.hpp"
#else
#include "choice.hpp"
#endif

double pi = 2.0*acos(0.0);

class ConstantXBoundary : public SpatialFilter
{
private:
  double xval;
public:
  ConstantXBoundary(double xval): xval(xval) {};
  bool matchesPoint(double x, double y)
  {
    double tol = 1e-14;
    return (abs(x-xval) < tol);
  }
};

class ConstantYBoundary : public SpatialFilter
{
private:
  double yval;
public:
  ConstantYBoundary(double yval): yval(yval) {};
  bool matchesPoint(double x, double y)
  {
    double tol = 1e-14;
    return (abs(y-yval) < tol);
  }
};

class ExactU1 : public Function
{
public:
  double lambda;
  ExactU1(double lambda) : Function(0), lambda(lambda) {}
  void values(FieldContainer<double> &values, BasisCachePtr basisCache)
  {
    int numCells = values.dimension(0);
    int numPoints = values.dimension(1);

    const FieldContainer<double> *points = &(basisCache->getPhysicalCubaturePoints());
    for (int cellIndex=0; cellIndex<numCells; cellIndex++)
    {
      for (int ptIndex=0; ptIndex<numPoints; ptIndex++)
      {
        double x = (*points)(cellIndex,ptIndex,0);
        double y = (*points)(cellIndex,ptIndex,1);
        values(cellIndex, ptIndex) = 1-exp(lambda*x)*cos(2*pi*y);
      }
    }
  }
};

class ExactU2 : public Function
{
public:
  double lambda;
  ExactU2(double lambda) : Function(0), lambda(lambda) {}
  void values(FieldContainer<double> &values, BasisCachePtr basisCache)
  {
    int numCells = values.dimension(0);
    int numPoints = values.dimension(1);

    const FieldContainer<double> *points = &(basisCache->getPhysicalCubaturePoints());
    for (int cellIndex=0; cellIndex<numCells; cellIndex++)
    {
      for (int ptIndex=0; ptIndex<numPoints; ptIndex++)
      {
        double x = (*points)(cellIndex,ptIndex,0);
        double y = (*points)(cellIndex,ptIndex,1);
        values(cellIndex, ptIndex) = lambda/(2*pi)*exp(lambda*x)*sin(2*pi*y);
      }
    }
  }
};

int main(int argc, char *argv[])
{
#ifdef HAVE_MPI
  Teuchos::GlobalMPISession mpiSession(&argc, &argv,0);
  choice::MpiArgs args( argc, argv );
#else
  choice::Args args( argc, argv );
#endif
  int commRank = Teuchos::GlobalMPISession::getRank();
  int numProcs = Teuchos::GlobalMPISession::getNProc();

  // Required arguments
  int numRefs = args.Input<int>("--numRefs", "number of refinement steps");
  int norm = args.Input<int>("--norm", "0 = graph\n    1 = robust\n    2 = coupled robust");

  // Optional arguments (have defaults)
  bool enforceLocalConservation = args.Input<bool>("--conserve", "enforce local conservation", false);
  double Re = args.Input("--Re", "Reynolds number", 40);
  double nu = 1./Re;
  double lambda = Re/2.-sqrt(Re*Re/4+4*pi*pi);
  int maxNewtonIterations = args.Input("--maxIterations", "maximum number of Newton iterations", 20);
  int polyOrder = args.Input("--polyOrder", "polynomial order for field variables", 2);
  int deltaP = args.Input("--deltaP", "how much to enrich test space", 2);
  // string saveFile = args.Input<string>("--meshSaveFile", "file to which to save refinement history", "");
  // string replayFile = args.Input<string>("--meshLoadFile", "file with refinement history to replay", "");
  args.Process();

  // if (commRank==0)
  // {
  //   cout << "saveFile is " << saveFile << endl;
  //   cout << "loadFile is " << replayFile << endl;
  // }

  ////////////////////   PROBLEM DEFINITIONS   ///////////////////////
  int H1Order = polyOrder+1;

  ////////////////////   DECLARE VARIABLES   ///////////////////////
  // define test variables
  VarFactory varFactory;
  // VarPtr tau11 = varFactory.testVar("tau11", HGRAD);
  // VarPtr tau12 = varFactory.testVar("tau12", HGRAD);
  // VarPtr tau22 = varFactory.testVar("tau22", HGRAD);
  VarPtr tau1 = varFactory.testVar("tau1", HDIV);
  VarPtr tau2 = varFactory.testVar("tau2", HDIV);
  VarPtr v1 = varFactory.testVar("v1", HGRAD);
  VarPtr v2 = varFactory.testVar("v2", HGRAD);
  VarPtr q = varFactory.testVar("q", HGRAD);

  // define trial variables
  VarPtr u1 = varFactory.fieldVar("u1");
  VarPtr u2 = varFactory.fieldVar("u2");
  // VarPtr sigma11 = varFactory.fieldVar("sigma11");
  // VarPtr sigma12 = varFactory.fieldVar("sigma12");
  // VarPtr sigma22 = varFactory.fieldVar("sigma22");
  VarPtr sigma1 = varFactory.fieldVar("sigma1", VECTOR_L2);
  VarPtr sigma2 = varFactory.fieldVar("sigma2", VECTOR_L2);
  VarPtr u1hat = varFactory.traceVar("u1hat");
  VarPtr u2hat = varFactory.traceVar("u2hat");
  VarPtr t1hat = varFactory.fluxVar("t1hat");
  VarPtr t2hat = varFactory.fluxVar("t2hat");
  VarPtr p = varFactory.fieldVar("p");

  ////////////////////   BUILD MESH   ///////////////////////
  BFPtr bf = Teuchos::rcp( new BF(varFactory) );

  // define nodes for mesh
  FieldContainer<double> meshBoundary(4,2);
  double xmin = -0.5;
  double xmax =  1.0;
  double ymin = -0.5;
  double ymax =  1.5;

  meshBoundary(0,0) =  xmin; // x1
  meshBoundary(0,1) =  ymin; // y1
  meshBoundary(1,0) =  xmax;
  meshBoundary(1,1) =  ymin;
  meshBoundary(2,0) =  xmax;
  meshBoundary(2,1) =  ymax;
  meshBoundary(3,0) =  xmin;
  meshBoundary(3,1) =  ymax;

  int horizontalCells = 6, verticalCells = 8;

  // create a pointer to a new mesh:
  Teuchos::RCP<Mesh> mesh = Mesh::buildQuadMesh(meshBoundary, horizontalCells, verticalCells,
                            bf, H1Order, H1Order+deltaP);

  ////////////////////////////////////////////////////////////////////
  // INITIALIZE BACKGROUND FLOW FUNCTIONS
  ////////////////////////////////////////////////////////////////////

  BCPtr nullBC = Teuchos::rcp((BC*)NULL);
  RHSPtr nullRHS = Teuchos::rcp((RHS*)NULL);
  IPPtr nullIP = Teuchos::rcp((IP*)NULL);
  SolutionPtr backgroundFlow = Teuchos::rcp(new Solution(mesh, nullBC, nullRHS, nullIP) );

  vector<double> e1(2); // (1,0)
  e1[0] = 1;
  vector<double> e2(2); // (0,1)
  e2[1] = 1;

  FunctionPtr u1_prev = Function::solution(u1, backgroundFlow);
  FunctionPtr u2_prev = Function::solution(u2, backgroundFlow);
  FunctionPtr sigma1_prev = Function::solution(sigma1, backgroundFlow);
  FunctionPtr sigma2_prev = Function::solution(sigma2, backgroundFlow);
  FunctionPtr p_prev = Function::solution(p, backgroundFlow);
  // FunctionPtr sigma11_prev = Function::solution(sigma11, backgroundFlow);
  // FunctionPtr sigma12_prev = Function::solution(sigma12, backgroundFlow);
  // FunctionPtr sigma22_prev = Function::solution(sigma22, backgroundFlow);

  FunctionPtr zero = Teuchos::rcp( new ConstantScalarFunction(0.0) );
  FunctionPtr one = Teuchos::rcp( new ConstantScalarFunction(1.0) );
  FunctionPtr u1Exact     = Teuchos::rcp( new ExactU1(lambda) );
  FunctionPtr u2Exact     = Teuchos::rcp( new ExactU2(lambda) );
  // FunctionPtr beta = e1 * u1_prev + e2 * u2_prev;

  // ==================== SET INITIAL GUESS ==========================
  map<int, Teuchos::RCP<Function> > functionMap;
  functionMap[u1->ID()] = u1Exact;
  functionMap[u2->ID()] = u2Exact;
  // functionMap[sigma1->ID()] = Function::vectorize(zero,zero);
  // functionMap[sigma2->ID()] = Function::vectorize(zero,zero);
  // functionMap[p->ID()] = zero;

  backgroundFlow->projectOntoMesh(functionMap);

  ////////////////////   DEFINE BILINEAR FORM   ///////////////////////

  // // stress equation
  bf->addTerm( 1./nu*sigma1, tau1 );
  bf->addTerm( 1./nu*sigma2, tau2 );
  bf->addTerm( u1, tau1->div() );
  bf->addTerm( u2, tau2->div() );
  bf->addTerm( -u1hat, tau1->dot_normal() );
  bf->addTerm( -u2hat, tau2->dot_normal() );
  // bf->addTerm( 1./(2*nu)*sigma11, tau11 );
  // bf->addTerm( 1./(2*nu)*sigma12, tau12 );
  // bf->addTerm( 1./(2*nu)*sigma12, tau12 );
  // bf->addTerm( 1./(2*nu)*sigma22, tau22 );
  // bf->addTerm( u1, tau11->dx() );
  // bf->addTerm( u1, tau12->dy() );
  // bf->addTerm( u2, tau12->dx() );
  // bf->addTerm( u2, tau22->dy() );
  // bf->addTerm( -u1hat, tau11->times_normal_x() );
  // bf->addTerm( -u1hat, tau12->times_normal_y() );
  // bf->addTerm( -u2hat, tau12->times_normal_x() );
  // bf->addTerm( -u2hat, tau22->times_normal_y() );

  // momentum equation
  bf->addTerm( -2.*u1_prev*u1, v1->dx() );
  bf->addTerm( -u2_prev*u1, v1->dy() );
  bf->addTerm( -u1_prev*u2, v1->dy() );
  bf->addTerm( -u2_prev*u1, v2->dx() );
  bf->addTerm( -u1_prev*u2, v1->dy() );
  bf->addTerm( -2.*u2_prev*u2, v2->dy() );
  bf->addTerm( -p, v1->dx() );
  bf->addTerm( -p, v2->dy() );
  // bf->addTerm( sigma11, v1->dx() );
  // bf->addTerm( sigma12, v1->dy() );
  // bf->addTerm( sigma12, v2->dx() );
  // bf->addTerm( sigma22, v2->dy() );
  bf->addTerm( sigma1, v1->grad() );
  bf->addTerm( sigma2, v2->grad() );
  bf->addTerm( t1hat, v1);
  bf->addTerm( t2hat, v2);

  // continuity equation
  bf->addTerm( -u1, q->dx() );
  bf->addTerm( -u2, q->dy() );
  bf->addTerm( u1hat, q->times_normal_x() );
  bf->addTerm( u2hat, q->times_normal_y() );

  ////////////////////   SPECIFY RHS   ///////////////////////
  Teuchos::RCP<RHSEasy> rhs = Teuchos::rcp( new RHSEasy );

  // stress equation
  rhs->addTerm( -u1_prev * tau1->div() );
  rhs->addTerm( -u2_prev * tau2->div() );

  // momentum equation
  rhs->addTerm( 2.*u1_prev*u1_prev * v1->dx() );
  rhs->addTerm( u2_prev*u1_prev    * v1->dy() );
  rhs->addTerm( u1_prev*u2_prev    * v1->dy() );
  rhs->addTerm( u2_prev*u1_prev    * v2->dx() );
  rhs->addTerm( u1_prev*u2_prev    * v1->dy() );
  rhs->addTerm( 2.*u2_prev*u2_prev * v2->dy() );
  // rhs->addTerm( p_prev             * v1->dx() );
  // rhs->addTerm( p_prev             * v2->dy() );
  // rhs->addTerm( -sigma1_prev       * v1->grad() );
  // rhs->addTerm( -sigma2_prev       * v2->grad() );

  // rhs->addTerm( -sigma11_prev * v1->dx() );
  // rhs->addTerm( -sigma12_prev * v1->dy() );
  // rhs->addTerm( -sigma12_prev * v2->dx() );
  // rhs->addTerm( -sigma22_prev * v2->dy() );

  // continuity equation
  rhs->addTerm( u1_prev * q->dx() );
  rhs->addTerm( u2_prev * q->dy() );

  ////////////////////   DEFINE INNER PRODUCT(S)   ///////////////////////
  IPPtr ip = Teuchos::rcp(new IP);
  if (norm == 0)
  {
    ip = bf->graphNorm();
  }
  else if (norm == 1)
  {
    // ip = bf->l2Norm();
  }

  ////////////////////   CREATE BCs   ///////////////////////
  Teuchos::RCP<BCEasy> bc = Teuchos::rcp( new BCEasy );
  // Teuchos::RCP<PenaltyConstraints> pc = Teuchos::rcp( new PenaltyConstraints );
  SpatialFilterPtr left = Teuchos::rcp( new ConstantXBoundary(-0.5) );
  SpatialFilterPtr right = Teuchos::rcp( new ConstantXBoundary(1) );
  SpatialFilterPtr top = Teuchos::rcp( new ConstantYBoundary(-0.5) );
  SpatialFilterPtr bottom = Teuchos::rcp( new ConstantYBoundary(1.5) );
  bc->addDirichlet(u1hat, left, u1Exact);
  bc->addDirichlet(u2hat, left, u2Exact);
  bc->addDirichlet(u1hat, right, u1Exact);
  bc->addDirichlet(u2hat, right, u2Exact);
  bc->addDirichlet(u1hat, top, u1Exact);
  bc->addDirichlet(u2hat, top, u2Exact);
  bc->addDirichlet(u1hat, bottom, u1Exact);
  bc->addDirichlet(u2hat, bottom, u2Exact);

  // zero mean constraint on pressure
  bc->addZeroMeanConstraint(p);

  // pc->addConstraint(u1hat*u2hat-t1hat == zero, top);
  // pc->addConstraint(u2hat*u2hat-t2hat == zero, top);

  Teuchos::RCP<Solution> solution = Teuchos::rcp( new Solution(mesh, bc, rhs, ip) );
  // solution->setFilter(pc);

  // if (enforceLocalConservation) {
  //   solution->lagrangeConstraints()->addConstraint(u1hat->times_normal_x() + u2hat->times_normal_y() == zero);
  // }

  // ==================== Register Solutions ==========================
  mesh->registerSolution(solution);
  mesh->registerSolution(backgroundFlow);

  // Teuchos::RCP< RefinementHistory > refHistory = Teuchos::rcp( new RefinementHistory );
  // mesh->registerObserver(refHistory);

  ////////////////////   SOLVE & REFINE   ///////////////////////
  double energyThreshold = 0.2; // for mesh refinements
  RefinementStrategy refinementStrategy( solution, energyThreshold );
  VTKExporter exporter(backgroundFlow, mesh, varFactory);
  stringstream outfile;
  outfile << "kovasznay" << "_" << 0;
  exporter.exportSolution(outfile.str());

  double nonlinearRelativeEnergyTolerance = 1e-5; // used to determine convergence of the nonlinear solution
  for (int refIndex=0; refIndex<=numRefs; refIndex++)
  {
    double L2Update = 1e10;
    int iterCount = 0;
    while (L2Update > nonlinearRelativeEnergyTolerance && iterCount < maxNewtonIterations)
    {
      solution->solve(false);
      double u1L2Update = solution->L2NormOfSolutionGlobal(u1->ID());
      double u2L2Update = solution->L2NormOfSolutionGlobal(u2->ID());
      L2Update = sqrt(u1L2Update*u1L2Update + u2L2Update*u2L2Update);

      // Check local conservation
      if (commRank == 0)
      {
        cout << "L2 Norm of Update = " << L2Update << endl;

        // if (saveFile.length() > 0) {
        //   std::ostringstream oss;
        //   oss << string(saveFile) << refIndex ;
        //   cout << "on refinement " << refIndex << " saving mesh file to " << oss.str() << endl;
        //   refHistory->saveToFile(oss.str());
        // }
      }

      // line search algorithm
      double alpha = 1.0;
      backgroundFlow->addSolution(solution, alpha);
      iterCount++;
    }

    if (commRank == 0)
    {
      stringstream outfile;
      outfile << "kovasznay" << "_" << refIndex+1;
      exporter.exportSolution(outfile.str());
    }

    if (refIndex < numRefs)
      refinementStrategy.refine(commRank==0); // print to console on commRank 0
  }

  return 0;
}

