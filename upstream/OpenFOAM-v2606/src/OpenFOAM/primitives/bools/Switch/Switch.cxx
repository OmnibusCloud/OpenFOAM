/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2025 OpenCFD Ltd.
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

#include "Switch.H"
#include "scalar.H"
#include "error.H"
#include "dictionary.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

// Number of names[]
constexpr unsigned char numNames = (1+Foam::Switch::switchType::INVALID);

// The names corresponding to the switchType enumerations
static const char* names[numNames] =
{
    "false", "true",
    "no",    "yes",
    "off",   "on",
    "none",  "any",
    "invalid",  //< Output representation only
};


// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace
{

template<class OS>
OS& printTokenError(OS& os, const Foam::token& tok)
{
    if (!tok.good())
    {
        os << "Bad token - could not get bool/switch";
    }
    else if (tok.isWord())
    {
        os << "Expected true/false, on/off... found " << tok.wordToken();
    }
    else
    {
        os << "Wrong token - expected bool/switch, found " << tok.info();
    }
    os << '\n';

    return os;
}

} // End anonymous namespace


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::Switch::switchType
Foam::Switch::parse(const char* s, size_t len) noexcept
{
    switch (len)
    {
        case 1:  // (0|1|f|t|n|y)
        {
            switch (s[0])
            {
                case '0': case 'f': return switchType::FALSE;
                case '1': case 't': return switchType::TRUE;
                case 'n': return switchType::NO;
                case 'y': return switchType::YES;
            }
            break;
        }
        case 2:  // (no|on)
        {
            switch (s[0])
            {
                case 'n': if (s[1] == 'o') return switchType::NO; break;
                case 'o': if (s[1] == 'n') return switchType::ON; break;
            }
            break;
        }
        case 3:  // (off|yes|any)
        {
            switch (s[0])
            {
                case 'a':  // (any)
                {
                    if (s[1] == 'n' && s[2] == 'y') return switchType::ANY;
                    break;
                }
                case 'o':  // (off)
                {
                    if (s[1] == 'f' && s[2] == 'f') return switchType::OFF;
                    break;
                }
                case 'y':  // (yes)
                {
                    if (s[1] == 'e' && s[2] == 's') return switchType::YES;
                    break;
                }
            }
            break;
        }
        case 4:  // (none|true)
        {
            switch (s[0])
            {
                case 'n':  // (none)
                {
                    if (s[1] == 'o' && s[2] == 'n' && s[3] == 'e')
                    {
                        return switchType::NONE;
                    }
                    break;
                }
                case 't':  // (true)
                {
                    if (s[1] == 'r' && s[2] == 'u' && s[3] == 'e')
                    {
                        return switchType::TRUE;
                    }
                    break;
                }
            }
            break;
        }
        case 5:  // (false)
        {
            switch (s[0])
            {
                case 'f':  // (false)
                {
                    if
                    (
                        s[1] == 'a' && s[2] == 'l'
                     && s[3] == 's' && s[4] == 'e'
                    )
                    {
                        return switchType::FALSE;
                    }
                    break;
                }
            }
            break;
        }
    }

    return switchType::INVALID;
}


inline Foam::Switch::switchType
Foam::Switch::parse(const char* s)
{
    if (s)
    {
        return parse(s, std::char_traits<char>::length(s));
    }
    else
    {
        return switchType::INVALID;
    }
}


// * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * * //

const char* Foam::Switch::name(bool b) noexcept
{
    return names[(b ? 1 : 0)];
}


Foam::Switch Foam::Switch::find(const char* s)
{
    return Switch(parse(s));
}


Foam::Switch Foam::Switch::find(const std::string& s) noexcept
{
    return Switch(parse(s.data(), s.size()));
}


Foam::Switch Foam::Switch::find(std::string_view s) noexcept
{
    return Switch(parse(s.data(), s.size()));
}


bool Foam::Switch::contains(std::string_view s) noexcept
{
    return (switchType::INVALID != parse(s.data(), s.size()));
}


Foam::Switch Foam::Switch::getOrDefault
(
    const word& key,
    const dictionary& dict,
    const Switch deflt
)
{
    return dict.getOrDefault<Switch>(key, deflt, keyType::LITERAL);
}


Foam::Switch Foam::Switch::getOrAddToDict
(
    const word& key,
    dictionary& dict,
    const Switch deflt
)
{
    return dict.getOrAdd<Switch>(key, deflt, keyType::LITERAL);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::Switch::Switch(const char* s)
:
    value_(parse(s))
{
    if (!s)
    {
        FatalErrorInFunction
            << "Cannot construct switch from nullptr" << nl
            << abort(FatalError);
    }
    else if (bad())
    {
        FatalErrorInFunction
            << "Unknown switch " << s << nl
            << abort(FatalError);
    }
}


Foam::Switch::Switch(const std::string& s)
:
    value_(parse(s.data(), s.size()))
{
    if (bad())
    {
        FatalErrorInFunction
            << "Unknown switch " << s.data() << nl
            << abort(FatalError);
    }
}


Foam::Switch::Switch(std::string_view s)
:
    value_(parse(s.data(), s.size()))
{
    if (bad())
    {
        FatalErrorInFunction
            << "Unknown switch " << s << nl
            << abort(FatalError);
    }
}


Foam::Switch::Switch(const char* s, bool allowBad)
:
    value_(parse(s))
{
    if (bad() && !allowBad)
    {
        if (!s)
        {
            FatalErrorInFunction
                << "Cannot construct switch from nullptr" << nl
                << abort(FatalError);
        }
        else
        {
            FatalErrorInFunction
                << "Unknown switch " << s << nl
                << abort(FatalError);
        }
    }
}


Foam::Switch::Switch(const std::string& s, bool allowBad)
:
    value_(parse(s.data(), s.size()))
{
    if (bad() && !allowBad)
    {
        FatalErrorInFunction
            << "Unknown switch " << s.data() << nl
            << abort(FatalError);
    }
}


Foam::Switch::Switch(const float val, const float tol)
:
    value_(switchType::FALSE)
{
    if (Foam::mag(val) > tol)
    {
        value_ = switchType::TRUE;
    }
}


Foam::Switch::Switch(const double val, const double tol)
:
    value_(switchType::FALSE)
{
    if (Foam::mag(val) > tol)
    {
        value_ = switchType::TRUE;
    }
}


Foam::Switch::Switch(const token& tok)
:
    value_(switchType::INVALID)
{
    if (tok.good())
    {
        if (tok.isBool())
        {
            (*this) = tok.boolToken();
        }
        else if (tok.isLabel())
        {
            (*this) = bool(tok.labelToken());
        }
        else if (tok.isWord())
        {
            const auto& s = tok.wordToken();
            value_ = parse(s.data(), s.size());
        }
        // Also handle float/double with tolerance?
    }
}


Foam::Switch::Switch
(
    const word& key,
    const dictionary& dict
)
:
    value_(switchType::INVALID)
{
    const token tok(dict.get<token>(key, keyType::LITERAL));

    Switch sw(tok);

    if (sw.good())
    {
        (*this) = sw;
    }
    else
    {
        printTokenError(FatalIOErrorInFunction(dict), tok)
            << exit(FatalIOError);
    }
}


Foam::Switch::Switch
(
    const word& key,
    const dictionary& dict,
    const Switch deflt,
    const bool warnOnly
)
:
    value_(deflt.value_)
{
    token tok;

    if (dict.readIfPresent<token>(key, tok, keyType::LITERAL))
    {
        Switch sw(tok);

        if (sw.good())
        {
            (*this) = sw;
        }
        else if (warnOnly)
        {
            printTokenError(IOWarningInFunction(dict), tok)
                << "using default " << deflt.c_str() << endl;
        }
        else
        {
            printTokenError(FatalIOErrorInFunction(dict), tok)
                << exit(FatalIOError);
        }
    }
}


Foam::Switch::Switch(Istream& is)
:
    value_(switchType::FALSE)
{
    is >> *this;
}


// * * * * * * * * * * * * * * * Member Functions * * * * * * * * * * * * * * //

void Foam::Switch::negate() noexcept
{
    if (value_ < switchType::INVALID)
    {
        // Toggle final bit. So NO <-> YES, OFF <-> ON ...
        value_ ^= 0x1;
    }
}


const char* Foam::Switch::c_str() const noexcept
{
    if (value_ < numNames)
    {
        return names[value_];
    }
    else
    {
        return names[switchType::INVALID];
    }
}


std::string Foam::Switch::str() const
{
    if (value_ < numNames)
    {
        return names[value_];
    }
    else
    {
        return names[switchType::INVALID];
    }
}


bool Foam::Switch::readIfPresent
(
    const word& key,
    const dictionary& dict
)
{
    return dict.readIfPresent<Switch>(key, *this, keyType::LITERAL);
}


// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

Foam::Istream& Foam::operator>>(Istream& is, Switch& sw)
{
    token tok(is);

    Switch val(tok);

    if (val.good())
    {
        sw = val;
        is.check(FUNCTION_NAME);
    }
    else
    {
        printTokenError(FatalIOErrorInFunction(is), tok)
            << exit(FatalIOError);
        is.setBad();
    }

    return is;
}


Foam::Ostream& Foam::operator<<(Ostream& os, const Switch sw)
{
    os << sw.c_str();
    return os;
}


// ************************************************************************* //
