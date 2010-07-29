// BEGIN LICENSE BLOCK
/*
Copyright � 2009 , UT-Battelle, LLC
All rights reserved

[FreeFermions, Version 1.0.0]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."
 
*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************


*/
// END LICENSE BLOCK
/** \ingroup DMRG */
/*@{*/

/*! \file GeometryLibrary.h
 *
 * handy geometries
 *
 */
#ifndef GEOMETRY_LIB_H
#define GEOMETRY_LIB_H
#include "IoSimple.h"

namespace FreeFermions {
	template<typename MatrixType>
	class GeometryLibrary {
			typedef typename MatrixType::value_type FieldType;
		public:
			enum {CHAIN,LADDER,FEAS};
			enum {OPTION_NONE,OPTION_PERIODIC};
			enum {DIRECTION_X=0,DIRECTION_Y=1,DIRECTION_XPY=2,DIRECTION_XMY=3};
			
			GeometryLibrary(size_t n,size_t geometryType,FieldType hopping = 1.0) :
				sites_(n),geometryType_(geometryType),hopping_(hopping)
			{
			}
			void setGeometry(MatrixType& t,size_t geometryOption=OPTION_NONE)
			{
				switch (geometryType_) {
					case CHAIN:
						setGeometryChain(t,geometryOption);
						break;
					case LADDER:
						setGeometryLadder(t,geometryOption);
						break;
					default:
						throw std::runtime_error("GeometryLibrary:: Unknown geometry type\n");
				}
			}
			
			void setGeometry(std::vector<MatrixType>& t,const std::string& filename,size_t geometryOption=OPTION_NONE)
			{
				if (geometryType_!=FEAS) throw std::runtime_error("Unsupported\n");
				size_t edof = 2; // 2 orbitals
				std::vector<FieldType> oneSiteHoppings;
				readOneSiteHoppings(oneSiteHoppings,filename);
	
				t.clear();
				for (size_t i=0;i<edof*edof;i++) { // 4 cases: aa ab ba and bb
					MatrixType oneT(sites_,sites_);
					setGeometryFeAs(oneT,i,oneSiteHoppings,geometryOption);
					if (!isHermitian(oneT,true)) throw std::runtime_error("Hopping matrix not hermitian\n");
					t.push_back(oneT);
				}
			}
			
			template<typename ComplexFieldType>
			void fourierTransform(std::vector<ComplexFieldType>& dest,const MatrixType& src) const
			{
				
				size_t n = src.n_row();
				if (n!=sites_) throw std::runtime_error("src must have the same number of sites as lattice\n");
				psimag::Matrix<ComplexFieldType> B(n,n);
				getFourierMatrix(B);
				for (size_t k=0;k<n;k++) {
					ComplexFieldType sum = 0.0;
					for (size_t i=0;i<n;i++) {
						for (size_t j=0;j<n;j++) {
							sum += std::conj(B(i,k)) * src(i,j) * B(j,k);
						}
					}
					dest[k] = sum;
				}
				 
			}
			
		private:
			void setGeometryChain(MatrixType& t,size_t geometryOption)
			{
				for (size_t i=0;i<sites_;i++) {
					for (size_t j=0;j<sites_;j++) {
						if (i-j==1 || j-i==1) t(i,j) = hopping_;
					}
				}
				if (geometryOption==OPTION_PERIODIC) t(0,sites_-1) = t(sites_-1,0) = hopping_;
			}
			
			void setGeometryLadder(MatrixType& t,size_t leg)
			{
				if (leg<2) throw std::runtime_error("GeometryLibrary:: ladder must have leg>1\n");
				for (size_t i=0;i<sites_;i++) {
					std::vector<size_t> v;
					ladderFindNeighbors(v,i,leg);
					for (size_t k=0;k<v.size();k++) {
						size_t j = v[k];
						t(i,j) = t(j,i) = hopping_;
					}
				}
			}
			
			void ladderFindNeighbors(std::vector<size_t>& v,size_t i,size_t leg)
			{
				v.clear();
				size_t k = i + 1;
				if (k<sites_ && ladderSameColumn(k,i,leg)) v.push_back(k);
				k = i + leg;
				if (k<sites_)  v.push_back(k);
				
				// careful with substracting because i and k are unsigned!
				if (i==0) return;
				k = i-1;
				if (ladderSameColumn(i,k,leg)) v.push_back(k);
				
				if (i<leg) return;
				k= i-leg;
				v.push_back(k);
			}
			
			bool ladderSameColumn(size_t i,size_t k,size_t leg)
			{
				size_t i1 = i/leg;
				size_t k1 = k/leg;
				if (i1 == k1) return true;
				return false;
			}
			
			// only 2 orbitals supported
			void setGeometryFeAs(MatrixType& t,size_t orborb,const std::vector<FieldType>& oneSiteHoppings,size_t geometryOption)
			{
				FieldType tx = oneSiteHoppings[orborb+DIRECTION_X*4];
				size_t length = size_t(sqrt(sites_));
				if ((length*length) != sites_) throw std::runtime_error("Lattice must be square for FeAs\n");
				for (size_t j=0;j<length;j++) {
					for (size_t i=0;i<length;i++) {
						if (i+1<length) t(i+1+j*length,i+j*length) = t(i+j*length,i+1+j*length) = tx;
						if (i>0) t(i-1+j*length,i+j*length) = t(i+j*length,i-1+j*length) = tx;
					}
					if (geometryOption==OPTION_PERIODIC) 
						t(j*length,length-1+j*length) = t(length-1+j*length,j*length) = tx;
				}

				FieldType ty = oneSiteHoppings[orborb+DIRECTION_Y*4];
				for (size_t i=0;i<length;i++) {
					for (size_t j=0;j<length;j++) {
						if (j>0) t(i+(j-1)*length,i+j*length) = t(i+j*length,i+(j-1)*length) = ty;
						if (j+1<length) t(i+(j+1)*length,i+j*length) = t(i+j*length,i+(j+1)*length) = ty;
					}
					if (geometryOption==OPTION_PERIODIC)  t(i,i+(length-1)*length) = t(i+(length-1)*length,i) = ty;
				}
				FieldType txpy = oneSiteHoppings[orborb+DIRECTION_XPY*4];
				FieldType txmy = oneSiteHoppings[orborb+DIRECTION_XMY*4];
				for (size_t i=0;i<length;i++) {
					for (size_t j=0;j<length;j++) {
						if (j+1<length && i+1<length)
							t(i+1+(j+1)*length,i+j*length) = t(i+j*length,i+1+(j+1)*length) = txpy;
						if (i+1<length && j>0)
							t(i+1+(j-1)*length,i+j*length) = t(i+j*length,i+1+(j-1)*length) = txmy;
						if (geometryOption!=OPTION_PERIODIC || i>0) continue;
						if (j+1<length)
							t((j+1)*length,length-1+j*length) = t(length-1+j*length,(j+1)*length) = txpy;
						if (j>0)
							t((j-1)*length,length-1+j*length) = t(length-1+j*length,(j-1)*length) = txmy;
					}
					if (geometryOption!=OPTION_PERIODIC) continue;
					if (i+1<length) {
						t(i+1,i+(length-1)*length) = t(i+(length-1)*length,i+1) = txpy;
						t(i+1+(length-1)*length,i) = t(i,i+1+(length-1)*length) = txmy;
					}
					if (i>0) continue;
					t(0,length-1+(length-1)*length) = t(length-1+(length-1)*length,0) = txpy;
					t(0+(length-1)*length,length-1) = t(length-1,0+(length-1)*length) = txmy;
				}
				
			}
			
			void readOneSiteHoppings(std::vector<FieldType>& v,
							const std::string& filename)
			{
				typename Dmrg::IoSimple::In io(filename);
				io.read(v,"hoppings");
			}
			
			template<typename MatrixComplexType>
			void getFourierMatrix(MatrixComplexType& B) const
			{
				typedef typename MatrixComplexType::value_type ComplexType;
				if (geometryType_!=FEAS) throw std::runtime_error("getFourierMatrix: unsupported\n");
				size_t n = B.n_row();
				size_t length = size_t(sqrt(n));
				if ((length*length) !=n) throw std::runtime_error("Only square lattices supported for FEAS\n");
				for (size_t i=0;i<n;i++) {
					size_t rx = i % length;
					size_t ry = i/length;
					for (size_t k=0;k<n;k++) {
						size_t kx = k % length;
						size_t ky = k / length;
						FieldType tmpx = rx*kx*2.0*M_PI/length;
						FieldType tmpy = ry*ky*2.0*M_PI/length;
						FieldType tmp1 = cos(tmpx+tmpy);
						FieldType tmp2 = sin(tmpx+tmpy);
						B(i,k) = ComplexType(tmp1,tmp2);
					}
				}
			}
			
			size_t sites_;
			size_t geometryType_;
			FieldType hopping_;
	}; // GeometryLibrary
} // namespace FreeFermions

/*@}*/
#endif //GEOMETRY_LIB_H
