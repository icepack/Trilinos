//@HEADER
// ************************************************************************
// 
//               Tpetra: Linear Algebra Services Package 
//                 Copyright 2011 Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ************************************************************************
//@HEADER

#ifndef __MatrixMarket_raw_hpp
#define __MatrixMarket_raw_hpp

#include "MatrixMarket_Banner.hpp"
#include "MatrixMarket_CoordDataReader.hpp"
#include "MatrixMarket_util.hpp"
#include "Teuchos_CommHelpers.hpp"
#include "Teuchos_StandardCatchMacros.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <stdexcept>

namespace Tpetra {
  namespace MatrixMarket {
    namespace Raw {

      /// \class Element
      /// \author Mark Hoemmen
      /// \brief One structural nonzero of a sparse matrix
      ///
      template<class Scalar, class Ordinal>
      class Element {
      public:
	//! Default constructor: an invalid structural nonzero element
	Element () : rowIndex_ (-1), colIndex_ (-1), value_ (0) {}

	//! A structural nonzero element at (i,j) with value Aij
	Element (const Ordinal i, const Ordinal j, const Scalar& Aij) :
	  rowIndex_ (i), colIndex_ (j), value_ (Aij) {}

	//! Ignore the nonzero value for comparisons.
	bool operator== (const Element& rhs) {
	  return rowIndex_ == rhs.rowIndex_ && colIndex_ == rhs.colIndex_;
	}

	//! Ignore the nonzero value for comparisons.
	bool operator!= (const Element& rhs) {
	  return ! (*this == rhs);
	}

	//! Lex order first by row index, then by column index.
	bool operator< (const Element& rhs) const {
	  if (rowIndex_ < rhs.rowIndex_)
	    return true;
	  else if (rowIndex_ > rhs.rowIndex_)
	    return false;
	  else { // equal
	    return colIndex_ < rhs.colIndex_;
	  }
	}

	void merge (const Element& rhs, const bool replace=false) {
	  if (rowIndex() != rhs.rowIndex() || colIndex() != rhs.colIndex())
	    throw std::logic_error("Can only merge elements at the same "
				   "location in the sparse matrix");
	  else if (replace)
	    value_ = rhs.value_;
	  else
	    value_ += rhs.value_;
	}

	Ordinal rowIndex() const { return rowIndex_; }
	Ordinal colIndex() const { return colIndex_; }
	Scalar value() const { return value_; }

      private:
	Ordinal rowIndex_, colIndex_;
	Scalar value_;
      };

      //! Print out an Element to the given output stream
      template<class Scalar, class Ordinal>
      std::ostream& 
      operator<< (std::ostream& out, const Element<Scalar, Ordinal>& elt) 
      {
	typedef Teuchos::ScalarTraits<Scalar> STS;
	// Non-Ordinal types are floating-point types.  In order not to
	// lose information when we print a floating-point type, we have
	// to set the number of digits to print.  C++ standard behavior
	// in the default locale seems to be to print only five decimal
	// digits after the decimal point; this does not suffice for
	// double precision.  We solve the problem of how many digits to
	// print more generally below.  It's a rough solution so please
	// feel free to audit and revise it.
	//
	// FIXME (mfh 01 Feb 2011) 
	// This really calls for the following approach:
	//
	// Guy L. Steele and Jon L. White, "How to print floating-point
	// numbers accurately", 20 Years of the ACM/SIGPLAN Conference
	// on Programming Language Design and Implementation
	// (1979-1999): A Selection, 2003.
	if (! STS::isOrdinal)
	  {
	    // std::scientific, std::fixed, and default are the three
	    // output states for floating-point numbers.  A reasonable
	    // user-defined floating-point type should respect these
	    // flags; hopefully it does.
	    out << std::scientific;

	    // Decimal output is standard for Matrix Market format.
	    out << std::setbase (10);

	    // Compute the number of decimal digits required for expressing
	    // a Scalar, by comparing with IEEE 754 double precision (16
	    // decimal digits, 53 binary digits).  This would be easier if
	    // Teuchos exposed std::numeric_limits<T>::digits10, alas.
	    const double numDigitsAsDouble = 
	      16 * ((double) STS::t() / (double) Teuchos::ScalarTraits<double>::t());
	    // Adding 0.5 and truncating is a portable "floor".
	    const int numDigits = static_cast<int> (numDigitsAsDouble + 0.5);

	    // Precision to which a Scalar should be written.
	    out << std::setprecision (numDigits);
	  }
	out << elt.rowIndex() << " " << elt.colIndex() << " " << elt.value();
	return out;
      }

      template<class Scalar, class Ordinal>
      class Adder {
      public:
	typedef Ordinal index_type;
	typedef Scalar value_type;
	typedef Element<Scalar, Ordinal> element_type;
	typedef typename std::vector<element_type>::size_type size_type;

	Adder () : numRows_(0), numCols_(0), numNonzeros_(0) {}

	//! Add an element to the sparse matrix at location (i,j) (one-based indexing).
	void operator() (const Ordinal i, const Ordinal j, const Scalar& Aij) {
	  // i and j are 1-based
	  elts_.push_back (element_type (i-1, j-1, Aij));
	  // Keep track of the rightmost column containing a nonzero,
	  // and the bottommost row containing a nonzero.  This gives us
	  // a lower bound for the dimensions of the matrix, and a check
	  // for the reported dimensions of the matrix in the Matrix
	  // Market file.
	  numRows_ = std::max(numRows_, i);
	  numCols_ = std::max(numCols_, j);
	  numNonzeros_++;
	}

	/// \brief Print the sparse matrix data.  
	///
	/// We always print the data sorted.  You may also merge
	/// duplicate entries if you prefer.
	/// 
	/// \param out [out] Output stream to which to print
	/// \param doMerge [in] Whether to merge entries before printing
	/// \param replace [in] If merging, whether to replace duplicate
	///   entries; otherwise their values are added together.
	void print (std::ostream& out, const bool doMerge, const bool replace=false) {
	  if (doMerge)
	    merge (replace);
	  else
	    std::sort (elts_.begin(), elts_.end());
	  // Print out the results, delimited by newlines.
	  typedef std::ostream_iterator<element_type> iter_type;
	  std::copy (elts_.begin(), elts_.end(), iter_type (out, "\n"));
	}

	/// \brief Merge duplicate elements 
	///
	/// Merge duplicate elements of the sparse matrix, where
	/// "duplicate" means at the same (i,j) location in the sparse
	/// matrix.  Resize the array of elements to fit just the
	/// "unique" (meaning "nonduplicate") elements.
	///
	/// \return (# unique elements, # removed elements)
	std::pair<size_type, size_type>
	merge (const bool replace=false) 
	{
	  typedef typename std::vector<element_type>::iterator iter_type;

	  // Start with a sorted container.  It may be sorted already,
	  // but we just do the extra work.  Element objects sort in
	  // lexicographic order of their (row, column) indices.
	  std::sort (elts_.begin(), elts_.end());

	  // Walk through the array in place, merging duplicates and
	  // pushing unique elements up to the front of the array.  We
	  // can't use std::unique for this because it doesn't let us
	  // merge duplicate elements; it only removes them from the
	  // sequence.
	  size_type numUnique = 0;
	  iter_type cur = elts_.begin();
	  if (cur == elts_.end())
	    // There are no elements to merge
	    return std::make_pair (numUnique, size_type(0));
	  else {
	    iter_type next = cur;
	    ++next; // There is one unique element
	    ++numUnique;
	    while (next != elts_.end()) {
	      if (*cur == *next) {
		// Merge in the duplicated element *next
		cur->merge (*next, replace);
	      } else {
		// *cur is already a unique element.  Move over one to
		// *make space for the new unique element.
		++cur; 
		*cur = *next; // Add the new unique element
		++numUnique;
	      }
	      // Look at the "next" not-yet-considered element
	      ++next;
	    }
	    // Remember how many elements we removed before resizing.
	    const size_type numRemoved = elts_.size() - numUnique;
	    elts_.resize (numUnique);
	    return std::make_pair (numUnique, numRemoved);
	  }
	}

      private:
	Ordinal numRows_, numCols_, numNonzeros_;
	std::vector<element_type> elts_;
      };


      template<class Scalar, class Ordinal>
      class Reader {
      public:
	/// \brief Read the sparse matrix from the given file.
	///
	/// This is a collective operation.  Only Rank 0 opens the
	/// file and reads data from it, but all ranks participate and
	/// wait for the final result.
	///
	/// \note This whole "raw" reader is meant for debugging and
	///   diagnostics of syntax errors in the Matrix Market file;
	///   it's not performance-oriented.  That's why we do all the
	///   broadcasts of and checks for "success".
	static bool
	readFile (const Teuchos::Comm<int>& comm,
		  const std::string& filename,
		  const bool echo,
		  const bool tolerant,
		  const bool debug=false)
	{
	  using std::cerr;
	  using std::endl;

	  const int myRank = Teuchos::rank (comm);
	  // Teuchos::broadcast doesn't accept a bool; we use an int
	  // instead, with the usual 1->true, 0->false Boolean
	  // interpretation.
	  int readFile = 0;
	  Teuchos::RCP<std::ifstream> in; // only valid on Rank 0
	  if (myRank == 0)
	    {
	      if (debug)
		cerr << "Attempting to open file \"" << filename 
		     << "\" on Rank 0...";
	      in = Teuchos::rcp (new std::ifstream (filename.c_str()));
	      if (! *in)
		{
		  readFile = 0;
		  if (debug)
		    cerr << "failed." << endl;
		}
	      else
		{
		  readFile = 1;
		  if (debug)
		    cerr << "succeeded." << endl;
		}
	    }
	  Teuchos::broadcast (comm, 0, &readFile);
	  TEST_FOR_EXCEPTION(! readFile, std::runtime_error,
			     "Failed to open input file \"" + filename + "\".");
	  // Only Rank 0 will try to dereference "in".
	  return read (comm, in, echo, tolerant, debug);
	}


	/// \brief Read the sparse matrix from the given input stream.
	///
	/// This is a collective operation.  Only Rank 0 reads from
	/// the given input stream, but all ranks participate and wait
	/// for the final result.
	///
	/// \note This whole "raw" reader is meant for debugging and
	///   diagnostics of syntax errors in the Matrix Market file;
	///   it's not performance-oriented.  That's why we do all the
	///   broadcasts of and checks for "success".
	static bool
	read (const Teuchos::Comm<int>& comm,
	      const Teuchos::RCP<std::istream>& in,
	      const bool echo,
	      const bool tolerant,
	      const bool debug=false)
	{
	  using std::cerr;
	  using std::endl;

	  const int myRank = Teuchos::rank (comm);
	  std::pair<bool, std::string> result;
	  int msgSize = 0; // Size of error message (if any)
	  if (myRank == 0)
	    {
	      if (in.is_null())
		{
		  result.first = false;
		  result.second = "Input stream is null on Rank 0";
		}
	      else
		{
		  if (debug)
		    cerr << "About to read from input stream on Rank 0" << endl;
		  result = readOnRank0 (*in, echo, tolerant, debug);
		  if (debug)
		    {
		      if (result.first)
			cerr << "Successfully read sparse matrix from "
			  "input stream on Rank 0" << endl;
		      else
			cerr << "Failed to read sparse matrix from input "
			  "stream on Rank 0" << endl;
		    }
		}
	      if (result.first)
		msgSize = 0;
	      else
		msgSize = result.second.size();
	    }
	  int success = result.first ? 1 : 0;
	  Teuchos::broadcast (comm, 0, &success);
	  if (! success)
	    {
	      if (! tolerant)
		{
		  // Tell all ranks how long the error message is, so
		  // they can make space for it in order to receive
		  // the broadcast of the error message.
		  Teuchos::broadcast (comm, 0, &msgSize);

		  if (msgSize > 0)
		    {
		      std::string errMsg (msgSize, ' ');
		      if (myRank == 0)
			std::copy (result.second.begin(), result.second.end(),
				   errMsg.begin());
		      Teuchos::broadcast (comm, 0, static_cast<int>(msgSize), 
					  static_cast<char* const> (&errMsg[0]));
		      TEST_FOR_EXCEPTION(! success, std::runtime_error, errMsg);
		    }
		  else
		    TEST_FOR_EXCEPTION(! success, std::runtime_error, 
				       "Unknown error when reading Matrix "
				       "Market sparse matrix file; the error "
				       "is \"unknown\" because the error "
				       "message has length 0.");
		}
	      else if (myRank == 0)
		{
		  using std::cerr;
		  using std::endl;
		  cerr << "The following error occurred when reading the "
		    "sparse matrix: " << result.second << endl;
		}
	    }
	  return success;
	}

      private:

        //! To be called only on MPI Rank 0.
	static std::pair<bool, std::string>
	readOnRank0 (std::istream& in,	
		     const bool echo,
		     const bool tolerant,
		     const bool debug=false)
	{
	  using std::cerr;
	  using std::cout;
	  using std::endl;
	  using Teuchos::RCP;
	  using Teuchos::Tuple;
	  typedef Teuchos::ScalarTraits<Scalar> STS;

	  std::ostringstream err;

	  if (! in)
	    {
	      err << "Input stream appears to be in an invalid state.";
	      return std::make_pair (false, err.str());
	    }
	  std::string line;
	  if (! getline (in, line))
	    {
	      err << "Failed to get first (banner) line";
	      return std::make_pair (false, err.str());
	    }

	  Banner banner (line, tolerant);
	  if (banner.matrixType() != "coordinate")
	    {
	      err << "Matrix Market input file must contain a "
		"\"coordinate\"-format sparse matrix in "
		"order to create a sparse matrix object "
		"from it.";
	      return std::make_pair (false, err.str());
	    }
	  else if (! STS::isComplex && banner.dataType() == "complex")
	    {
	      err << "Matrix Market file contains complex-"
		"valued data, but your chosen Scalar "
		"type is real.";
	      return std::make_pair (false, err.str());
	    }
	  else if (banner.dataType() != "real" && banner.dataType() != "complex")
	    {
	      err << "Only real or complex data types (no pattern or integer "
		"matrices) are currently supported.";
	      return std::make_pair (false, err.str());
	    }
	  if (debug)
	    {
	      cout << "Banner line:" << endl
		   << banner << endl;
	    }
	  // The rest of the file starts at line 2, after the banner line.
	  size_t lineNumber = 2;

	  // Make an "Adder" that knows how to add sparse matrix entries,
	  // given a line of data from the file.
	  typedef Adder<Scalar, Ordinal> raw_adder_type;
	  // SymmetrizingAdder "advices" (yes, I'm using that as a verb)
	  // the original Adder, so that additional entries are filled
	  // in symmetrically, if the Matrix Market banner line
	  // specified a symmetry type other than "general".
	  typedef SymmetrizingAdder<raw_adder_type> adder_type;
	  RCP<raw_adder_type> rawAdder (new raw_adder_type);
	  adder_type adder (rawAdder, banner.symmType());

	  if (banner.dataType() == "complex" && ! STS::isComplex)
	    {
	      err << "The Matrix Market sparse matrix file contains "
		"complex-valued data, but you are trying to read"
		" the data into a sparse matrix of real values.";
	      return std::make_pair (false, err.str());
	    }
	  // Make a reader that knows how to read "coordinate" format
	  // sparse matrix data.
	  typedef CoordDataReader<adder_type, Ordinal, Scalar, STS::isComplex> reader_type;
	  reader_type reader (adder);

	  // Read in the dimensions of the sparse matrix:
	  // (# rows, # columns, # structural nonzeros).
	  // The second element of the pair tells us whether the values
	  // were gotten successfully.
	  std::pair<Teuchos::Tuple<Ordinal, 3>, bool> dims = 
	    reader.readDimensions (in, lineNumber, tolerant);
	  if (! dims.second)
	    {
	      err << "Error reading Matrix Market sparse matrix "
		"file: failed to read coordinate dimensions.";
	      return std::make_pair (false, err.str());
	    }
	  if (debug)
	    {
	      const Ordinal numRows = dims.first[0];
	      const Ordinal numCols = dims.first[1];
	      const Ordinal numNonzeros = dims.first[2];
	      cout << "Dimensions of matrix: " << numRows << " x " << numCols 
		   << ", with " << numNonzeros << " reported structural "
		"nonzeros." << endl;
	    }

	  // Read the sparse matrix entries.  "results" just tells us if
	  // and where there were any bad lines of input.  The actual
	  // sparse matrix entries are stored in the Adder object.
	  std::pair<bool, std::vector<size_t> > results = 
	    reader.read (in, lineNumber, tolerant, debug);
	  if (debug)
	    {
	      if (results.first)
		cout << "Matrix Market file successfully read" << endl;
	      else 
		cout << "Failed to read Matrix Market file" << endl;
	    }

	  // Report any bad line number(s).
	  if (! results.first)
	    {
	      if (debug)
		reportBadness (cerr, results);
	      if (! tolerant)
		{
		  err << "Invalid Matrix Market file";
		  return std::make_pair (false, err.str());
		}
	    }
	  // We're done reading in the sparse matrix.  Now print out the
	  // nonzero entry/ies.
	  if (echo)
	    {
	      const bool doMerge = false;
	      const bool replace = false;
	      rawAdder->print (cout, doMerge, replace);
	    }
	  cout << endl;

	  return std::make_pair (true, err.str());
	}

        //! To be called only on MPI Rank 0.
	static void 
	reportBadness (std::ostream& out, 
		       const std::pair<bool, std::vector<size_t> >& results) 
	{
	  using std::endl;
	  const size_t numErrors = results.second.size();
	  const size_t maxNumErrorsToReport = 100;
	  out << numErrors << " errors when reading Matrix Market sparse "
	    "matrix file." << endl;
	  if (numErrors > maxNumErrorsToReport)
	    out << "-- We do not report individual errors when there "
	      "are more than " << maxNumErrorsToReport << ".";
	  else if (numErrors == 1)
	    out << "Error on line " << results.second[0] << endl;
	  else if (numErrors > 1)
	    {
	      out << "Errors on lines {";
	      for (size_t k = 0; k < numErrors-1; ++k)
		out << results.second[k] << ", ";
	      out << results.second[numErrors-1] << "}" << endl;
	    }
	}
      };

    } // namespace Raw
  } // namespace MatrixMarket
} // namespace Tpetra

#endif // __MatrixMarket_raw_hpp
