/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2025 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "bitSet.H"
#include "Switch.H"
#include "IOstreams.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

// Format is either of these:
//
//   <int1> <int2> ( ... )
//   <int1> <int2> { <bool> }
// where int1 = size, int2 = number of 'on' entries
// - within the brackets : the locations of the 'on' entries.
// - within the braces   : a uniform set where true = all, false = none
//
Foam::Ostream& Foam::bitSet::writeListToc
(
    Ostream& os,
    label shortLen
) const
{
    const bitSet& bitset = *this;
    if (shortLen < 0) shortLen = 1;  // <- sanity

    // Size and number of 'ON' entries:
    const label len = bitset.size();
    const label tocLen = (bitset.any() ? bitset.count() : 0);

    Switch uniformity =
    (
        (tocLen == 0)   ? Switch::FALSE  // None on
      : (tocLen == len) ? Switch::TRUE   // All on
      : Switch::INVALID  // Mixed
    );


    // Not yet:
    // if (os.format() == IOstreamOption::BINARY) {}

    if (len > 1 && uniformity.good())
    {
        // Two or more entries, and all entries have identical values

        // Count/size and delimiter
        os  << len << token::SPACE << tocLen << token::BEGIN_BLOCK;

        if (uniformity)
        {
            os << word("true");
        }
        else
        {
            os << word("false");
        }
        os << token::END_BLOCK;

    }
    else if (len <= 1 || !shortLen || (len <= shortLen))
    {
        // Single-line output

        // Count/size and delimiter
        os  << len << token::SPACE << tocLen << token::BEGIN_LIST;

        label iter = bitset.find_first();

        if (iter >= 0)
        {
            os << iter;
            iter = bitset.find_next(iter);
        }

        // Contents
        for (/*nil*/; iter >= 0; iter = bitset.find_next(iter))
        {
            os << token::SPACE << iter;
        }

        // End delimiter
        os << token::END_LIST;
    }
    else
    {
        // Multi-line output, but with linebreaks according to shortLen

        // Count/size and delimiter
        os << len << token::SPACE << tocLen << token::BEGIN_LIST;

        label column = 0;  // The current output position

        // Contents
        for
        (
            label iter = bitset.find_first();
            iter >= 0;
            iter = bitset.find_next(iter)
        )
        {
            if (column >= shortLen) { os << nl; }
            else if (column) { os << token::SPACE; }
            ++column;
            os  << iter;
        }

        // End delimiter
        os << nl << token::END_LIST;
    }

    return os;
}


Foam::Istream& Foam::bitSet::readListToc(Istream& is)
{
    bitSet& bitset = *this;

    // Input errors that may be encountered:
    enum errorType
    {
        ErrorNone = 0,  // OK
        ErrorToken1,    // Bad first token
        ErrorToken2,    // Bad second token
        ErrorSizing,    // Bad size (len < tocLen)
        ErrorBounds,    // Bad toc entry
    };
    int errorCode(errorType::ErrorNone);

    label len(0);
    label tocLen(0);
    label tocEntry(-1);  // The toc entry while reading


    is.fatalCheck(FUNCTION_NAME);

    token tok(is);
    is.fatalCheck("bitSet::readTocList() : reading first token");

    if (tok.isLabel())
    {
        len = tok.labelToken();
        tok.read(is);

        if (tok.isLabel())
        {
            tocLen = tok.labelToken();
            if (len < tocLen) { errorCode = errorType::ErrorSizing; }
        }
        else
        {
            errorCode = errorType::ErrorToken2;
        }
    }
    else
    {
        errorCode = errorType::ErrorToken1;
    }

    if (errorCode == errorType::ErrorNone) do
    {
        bitset.clear();  // Clear old contents
        bitset.resize(len);

        // Not yet:
        // if (is.format() == IOstreamOption::BINARY) {} else
        {
            // Begin of contents marker
            const char delimiter = is.readBeginList("bitSet");

            if (delimiter == token::BEGIN_LIST)
            {
                // Read entries
                for (label i = 0; i < tocLen; ++i)
                {
                    is >> tocEntry;
                    is.fatalCheck
                    (
                        "bitSet::readCompact(Istream&) : "
                        "reading entry"
                    );

                    if (tocEntry < 0 || tocEntry >= len)
                    {
                        errorCode = errorType::ErrorBounds;
                        break;
                    }
                    else
                    {
                        bitset.set(tocEntry);
                    }
                }
            }
            else
            {
                // Uniform content (delimiter == token::BEGIN_BLOCK)
                Switch uniformity(is);

                is.fatalCheck
                (
                    "bitSet::readTocList() : "
                    "reading the single entry"
                );

                // Fill with the value
                bitset.fill(uniformity);
            }

            // End of contents marker

            if (errorCode == errorType::ErrorNone)
            {
                is.readEndList("bitSet");
            }
        }
    }
    while (false);

    if (errorCode != errorType::ErrorNone)
    {
        auto& err = FatalIOErrorInFunction(is);

        if (errorType::ErrorToken1 == errorCode)
        {
            err << "Incorrect first token [size], expected <int>, found "
                << tok.info() << nl;
        }
        else if (errorType::ErrorToken2 == errorCode)
        {
            err << "Incorrect second token [toc size], expected <int>, found "
                << tok.info() << nl;
        }
        else if (errorType::ErrorSizing == errorCode)
        {
            err << "Bad sizing: has " << tocLen
                << " entries for a bitSet with size " << len << nl;
        }
        else if (errorType::ErrorBounds == errorCode)
        {
            err << "Entry " << tocEntry
                << " not within the expected bounds [0," << len << "]" << nl;
        }
        else
        {
            err << "Unspecified error" << nl;
        }

        err << exit(FatalIOError);
    }

    return is;
}


// * * * * * * * * * * * * * * Ostream Operators * * * * * * * * * * * * * * //

Foam::Ostream& Foam::operator<<(Ostream& os, const bitSet& bitset)
{
    return bitset.writeList(os, 40);
}


Foam::Ostream& Foam::operator<<
(
    Ostream& os,
    const InfoProxy<bitSet>& iproxy
)
{
    const auto& bitset = *iproxy;

    os  << "bitSet<" << bitSet::elem_per_block
        << "> size=" << bitset.size() << '/' << bitset.capacity()
        << " count=" << bitset.count()
        << nl;

    return os;
}


// ************************************************************************* //
