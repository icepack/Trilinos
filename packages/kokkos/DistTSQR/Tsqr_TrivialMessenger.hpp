// @HEADER
// ***********************************************************************
//
//                 Anasazi: Block Eigensolvers Package
//                 Copyright (2010) Sandia Corporation
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

#ifndef __TSQR_TrivialMessenger_hpp
#define __TSQR_TrivialMessenger_hpp

#include <Tsqr_MessengerBase.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace TSQR { 

  /// \class TrivialMessenger
  /// \brief Noncommunicating "communication" object for TSQR
  ///
  /// The internode parallel part of TSQR communicates via a
  /// MessengerBase< Datum > interface.  TrivialMessenger< Datum >
  /// implements that interface by acting as if running on MPI with
  /// only one rank, though it doesn't require MPI support to build.
  ///
  /// \warning Datum should be a class with value-type semantics.
  template< class Datum >
  class TrivialMessenger : public MessengerBase< Datum > {
  public:
    TrivialMessenger () {}

    /// Send sendData[0:sendCount-1] to process destProc.
    ///
    /// \param sendData [in] Array of value-type elements to send
    /// \param sendCount [in] Number of elements in the array
    /// \param destProc [in] Rank of destination process
    /// \param tag [in] MPI tag (ignored)
    virtual void 
    send (const Datum sendData[], 
	  const int sendCount, 
	  const int destProc, 
	  const int tag) 
    {}

    /// Receive recvData[0:recvCount-1] from process srcProc.
    ///
    /// \param recvData [out] Array of value-type elements to receive
    /// \param recvCount [in] Number of elements to receive in the array
    /// \param srcProc [in] Rank of sending process
    /// \param tag [in] MPI tag (ignored)
    virtual void 
    recv (Datum recvData[], 
	  const int recvCount, 
	  const int srcProc, 
	  const int tag) 
    {}

    /// Exchange sencRecvCount elements of sendData with processor
    /// destProc, receiving the result into recvData.  Assume that
    /// sendData and recvData do not alias one another.
    ///
    /// \param sendData [in] Array of value-type elements to send
    /// \param recvData [out] Array of value-type elements to
    ///   receive.  Caller is responsible for making sure that
    ///   recvData does not alias sendData.
    /// \param sendRecvCount [in] Number of elements to send and
    ///   receive in the array
    /// \param destProc [in] The "other" process' rank (to which
    ///   this process is sending data, and from which this process is
    ///   receiving data)
    /// \param tag [in] MPI tag (ignored)
    virtual void 
    swapData (const Datum sendData[], 
	      Datum recvData[], 
	      const int sendRecvCount, 
	      const int destProc, 
	      const int tag)
    {
      if (destProc != rank())
	{
	  std::ostringstream os;
	  os << "Destination rank " << destProc << " is invalid.  The only "
	     << "valid rank for TSQR::TrivialMessenger is 0 (zero).";
	    throw std::invalid_argument (os.str());
	}
      else if (sendRecvCount < 0)
	{
	  std::ostringstream os;
	  os << "sendRecvCount = " << sendRecvCount << " is invalid: "
	     << "only nonnegative values are allowed.";
	  throw std::invalid_argument (os.str());
	}
      else if (sendRecvCount == 0)
	return; // No data to exchange
      else 
	safeCopy (sendData, recvData, sendRecvCount);
    }


    /// Allreduce sum all processors' inDatum data, returning the
    /// result (on all processors).
    virtual Datum 
    globalSum (const Datum& inDatum) 
    {
      Datum outDatum (inDatum);
      return outDatum;
    }

    ///
    /// Assumes that Datum objects are less-than comparable by the
    /// underlying communication protocol.
    ///
    virtual Datum 
    globalMin (const Datum& inDatum)
    {
      Datum outDatum (inDatum);
      return outDatum;
    }

    ///
    /// Assumes that Datum objects are less-than comparable by the
    /// underlying communication protocol.
    ///
    virtual Datum 
    globalMax (const Datum& inDatum)
    {
      Datum outDatum (inDatum);
      return outDatum;
    }

    /// Allreduce sum all processors' inData[0:count-1], storing the
    /// results (on all processors) in outData.
    virtual void
    globalVectorSum (const Datum inData[], 
		     Datum outData[], 
		     const int count) 
    {
      safeCopy (inData, outData, count);
    }

    virtual void
    broadcast (Datum data[], 
	       const int count,
	       const int root)
    {}

    /// 
    /// Return this process' rank.
    /// 
    virtual int rank () const { return 0; }

    /// 
    /// Return the total number of ranks in the communicator.
    /// 
    virtual int size () const { return 1; }

    /// 
    /// Execute a barrier over the communicator.
    /// 
    virtual void barrier () const { }

  private:

    /// Copy count elements of inData into outData.  Use a method
    /// appropriate for whether or not the inData and outData arrays
    /// alias one another.
    void
    safeCopy (const Datum inData[],
	      Datum outData[],
	      const int count) 
    {
      // Check for nonaliasing of inData and outData.
      if (&inData[count-1] < &outData[0] ||
	  &outData[count-1] < &inData[0])
	// The arrays don't overlap, so we can call std::copy.
	// std::copy assumes that the third argument does not
	// point to an element in the range of the first two
	// arguments.
	std::copy (inData, inData+count, outData);
      else
	{
	  // If inData and outData do alias one another, use
	  // the buffer as intermediate scratch space.
	  buf_.resize (count);
	  std::copy (inData, inData+count, buf_.begin());
	  std::copy (buf_.begin(), buf_.end(), outData);
	}
    }

    /// Buffer to guard against incorrect behavior for aliased arrays.
    /// Space won't be allocated unless needed.
    ///
    std::vector< Datum > buf_;
  };
} // namespace TSQR

#endif // __TSQR_TrivialMessenger_hpp
