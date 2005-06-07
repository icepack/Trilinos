//
// @HEADER
// ***********************************************************************
// 
//                           Rythmos Package
//                 Copyright (2005) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

namespace Rythmos {

//-----------------------------------------------------------------------------
// Function      : ForwardEuler::ForwardEuler
// Purpose       : constructor
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/26/05
//-----------------------------------------------------------------------------
ForwardEuler::ForwardEuler(Teuchos::RefCountPtr<Rythmos::NonlinearModel<Scalar> > &model)
{
  model_ = model;
  t_ = 0.0; // this should come from Scalar class
  solution_vector_ = (*model_).get_vector();
  residual_vector_ = (*model_).get_vector();
}
//
//-----------------------------------------------------------------------------
// Function      : ForwardEuler::ForwardEuler
// Purpose       : constructor
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/26/05
//-----------------------------------------------------------------------------
ForwardEuler::ForwardEuler()
{
}

//-----------------------------------------------------------------------------
// Function      : ~ForwardEuler::ForwardEuler
// Purpose       : destructor
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/26/05
//-----------------------------------------------------------------------------
ForwardEuler::~ForwardEuler()
{
}

//-----------------------------------------------------------------------------
// Function      : ForwardEuler::TakeStep
// Purpose       : Take a step 
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/26/05
//-----------------------------------------------------------------------------
Scalar ForwardEuler::TakeStep()
{
  // print something out about this method not supporting automatic variable step-size
  return(-1);
}

//-----------------------------------------------------------------------------
// Function      : ForwardEuler::TakeStep
// Purpose       : Take a step no larger than dt
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/26/05
//-----------------------------------------------------------------------------
Scalar ForwardEuler::TakeStep(Scalar dt)
{
  InArgs<Scalar> inargs;
  OutArgs<Scalar> outargs;

  inargs.set_x(solution_vector_);
  inargs.set_t(t_+dt);

  outargs.set_F(residual_vector_);

  (*problem_).evalModel(inargs,outargs);

  // solution_vector = solution_vector + dt*residual_vector
  Thyra::Vp_StV(&*solution_vector_,dt,*residual_vector_); 
  t_ += dt;

  return(dt);
}

} // namespace Rythmos
