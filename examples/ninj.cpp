// SAmple of how to use FreeFermions core engine to calculate
// < n_i n_j >

#include <cstdlib>
#include <unistd.h>
#include "Engine.h"
#include "GeometryLibrary.h"
#include "ConcurrencySerial.h"
#include "TypeToString.h"
#include "CreationOrDestructionOp.h"
#include "HilbertState.h"
#include "GeometryParameters.h"
#include "Tokenizer.h"
#include "LibraryOperator.h"

typedef double RealType;
typedef std::complex<double> ComplexType;
typedef RealType FieldType;
typedef PsimagLite::ConcurrencySerial<RealType> ConcurrencyType;
typedef PsimagLite::Matrix<RealType> MatrixType;
typedef FreeFermions::GeometryParameters<RealType> GeometryParamsType;
typedef FreeFermions::GeometryLibrary<MatrixType,GeometryParamsType> GeometryLibraryType;
typedef FreeFermions::Engine<RealType,FieldType,ConcurrencyType> EngineType;
typedef FreeFermions::CreationOrDestructionOp<EngineType> OperatorType;
typedef FreeFermions::HilbertState<OperatorType> HilbertStateType;
typedef OperatorType::FactoryType OpNormalFactoryType;
typedef FreeFermions::LibraryOperator<OperatorType> LibraryOperatorType;
typedef LibraryOperatorType::FactoryType OpLibFactoryType;

void usage(const std::string& thisFile)
{
	std::cout<<thisFile<<": USAGE IS "<<thisFile<<" ";
	std::cout<<" -n sites -e electronsUp -g geometry,[leg,filename]\n";
}
	
void readPotential(std::vector<RealType>& v,const std::string& filename)
{
	std::vector<RealType> w;
	PsimagLite::IoSimple::In io(filename);
	try {
		io.read(w,"PotentialT");
	} catch (std::exception& e) {
		std::cerr<<"INFO: No PotentialT in file "<<filename<<"\n";
	}
	io.rewind();
	
	io.read(v,"potentialV");
	if (w.size()==0) return;
	if (v.size()>w.size()) v.resize(w.size());
	for (size_t i=0;i<w.size();i++) v[i] += w[i];
}

void setMyGeometry(GeometryParamsType& geometryParams,const std::vector<std::string>& vstr)
{
	// default value
	geometryParams.type = GeometryLibraryType::CHAIN;
		
	if (vstr.size()<2) {
		// assume chain
		return;
	}

	std::string gName = vstr[0];
	if (gName == "chain") {
		throw std::runtime_error("setMyGeometry: -g chain takes no further arguments\n");
	}
	
	geometryParams.leg = atoi(vstr[1].c_str());
	
	if (gName == "ladder") {
		if (vstr.size()!=3) {
			usage("setMyGeometry");
			throw std::runtime_error("setMyGeometry: usage is: -g ladder,leg,isPeriodic \n");
		}
		geometryParams.type = GeometryLibraryType::LADDER;
		geometryParams.hopping.resize(2);
		geometryParams.hopping[0] =  geometryParams.hopping[1]  = 1.0;
		geometryParams.isPeriodicY = (atoi(vstr[2].c_str())>0);
		return;
	}
	
	if (vstr.size()!=3) {
			usage("setMyGeometry");
			throw std::runtime_error("setMyGeometry: usage is: -g {feas | ktwoniffour} leg filename\n");
	}

	geometryParams.filename = vstr[2];

	if (gName == "feas") {
		geometryParams.type = GeometryLibraryType::FEAS;
		return;
	}
	
	if (gName == "kniffour") {
		geometryParams.type = GeometryLibraryType::KTWONIFFOUR;
		return;
	}
}

int main(int argc,char* argv[])
{
	size_t n = 0;
	size_t electronsUp = 0;
	std::vector<RealType> v;
	GeometryParamsType geometryParams;
	std::vector<std::string> str;
	int opt = 0;
	
	geometryParams.type = GeometryLibraryType::CHAIN;
	
	while ((opt = getopt(argc, argv, "n:e:g:p:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
				v.resize(n,0);
				geometryParams.sites = n;
				break;
			case 'e':
				electronsUp = atoi(optarg);
				break;
			case 'g':
				PsimagLite::tokenizer(optarg,str,",");
				setMyGeometry(geometryParams,str);
				break;
			case 'p':
				readPotential(v,optarg);
				break;
			default: /* '?' */
				usage("setMyGeometry");
				throw std::runtime_error("Wrong usage\n");
		}
	}
	if (v.size()==4*n) {
		v.resize(2*n);
	}
	if (n==0 || geometryParams.sites==0) {
		usage("setMyGeometry");
		throw std::runtime_error("Wrong usage\n");
	}

	size_t dof = 1; // spinless

	GeometryLibraryType geometry(geometryParams);

 	geometry.addPotential(v);
	std::cerr<<geometry;
	ConcurrencyType concurrency(argc,argv);
	EngineType engine(geometry,concurrency,dof,true);
	std::vector<size_t> ne(dof,electronsUp); // n. of up (= n. of  down electrons)
	HilbertStateType gs(engine,ne);
	RealType sum = 0;
	for (size_t i=0;i<ne[0];i++) sum += engine.eigenvalue(i);
	std::cerr<<"Energy="<<dof*sum<<"\n";	

	size_t sigma = 0;
	size_t tot = n; //(debug) ? 1 : n;
	for (size_t site = 0; site<tot ; site++) {
		OpLibFactoryType opLibFactory(engine);
		LibraryOperatorType& myOp = opLibFactory(LibraryOperatorType::N,site,sigma);
		for (size_t site2=0; site2<tot; site2++) {
			HilbertStateType phi = gs;
			myOp.applyTo(phi);
			LibraryOperatorType& myOp2 = opLibFactory(LibraryOperatorType::N,site2,sigma);
			myOp2.applyTo(phi);
			std::cout<<scalarProduct(gs,phi)<<" ";
		}
		std::cout<<"\n";
	}
}

