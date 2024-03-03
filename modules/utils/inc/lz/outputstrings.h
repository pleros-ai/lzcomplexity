/***************************************************************************
                          outputstrings.h  -  description
                             -------------------
    begin                : 05 Oct 2018
    last modified        : 05 Oct 2018
    email                : estevez@fisica.uh.cu
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013-2018 by Ernesto Estevez Rams   						   *
 *   estevez@fisica.uh.cu   											   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef OUTPUTSTRINGS_H_INCLUDED
#define OUTPUTSTRINGS_H_INCLUDED

//...................................................
// This are the strings used in the output functions
//...................................................

#define INPUTFILE "[Input file: "
#define SINPUTFILE "[Second input file: "
#define LENGTH "[Length: "
#define EPSILON "[LZ76 epsilon factor: "
#define DENSITY "[Density: "
#define ALPHABETSIZETEXT "[Alphabet Size: "
#define SEQUENCE "[Sequence "
#define FACTORIZATION "[Factorization "
#define CUMULATIVELZ76 "[Cumulative LZ76: "
#define ASYMPTENTROPYTABLE "[Complexity density table: "
#define BRUIJNENTROPYTABLE "[De Bruijn normalized complexity table: "
#define RNDENTROPYTABLE "[RND normalized complexity table: "
#define LZ76COMPLEXITY "[LZ76: "
#define SHANNONENTROPYFACTOR "[Shannon entropy over factor sizes: "
#define MAXPHRASESIZE "[Maximum factor size: "
#define AVERAGEPHRASESIZE "[Average factor size: "
#define SIGMAPHRASESIZE "[Standard deviation over factor size: "
#define ASYMPTENTROPY "[LZ76 Entropy density: "
#define SIGMAASYMPTENTROPY "[LZ76 Standard deviation entropy density: "
#define STANDARDDEVIATION "[LZ76 Entropy density standard deviation: "
#define FITTEDENTROPY "[Fitted entropy density: "
#define RNDENTROPY "[RND Normalized Complexity: "
#define BRUIJNENTROPY "[DBruijn Normalized Complexity: "
#define NLZDISTANCE "[LZ76 non-normalized distance between successive lines: "
#define LZDISTANCE "[Normalized LZ76 distance between successive lines: "
#define MUTUALINFORMATION "[Mutual information between successive lines: "
#define MAXITER "[Maximum iteration for excess entropy calculation: "
#define EXCESSENTROPY "[LZ76 Excess entropy shuffling: "
#define EXCESSENTROPYMI "[LZ76 Excess entropy as mutual information: "
#define REDUNDANCY "[Redundancy: "
#define EXCESSENTROPYTABLE "[LZ76 Excess entropy table: "
#define EXCESSENTROPYLINE "[LZ76 Excess entropy table line: "
#define MULTIINFORMATION "[LZ76 Multi information: "
#define HAMMING "[Hamming compare: "
#define NLZCOMPARE "[Non-normalized DLZ compare: "
#define DLZCOMPARE "[Normalized DLZ compare: "
#define REDUNDANCYCOMPARE "[Redundancy compare: "
#define MICOMPARE "[Mutual Information compare: "
#define CONDH "[Normalized conditional entropy: "
#define FCOMPARE "[Comparing inputs: "

#define LENGTH_COUT "Length: "
#define EPSILON_COUT "Epsilon factor: "
#define DENSITY_COUT "Density: "
#define ALPHABETSIZE_COUT "Alphabet Size: "
#define SEQUENCE_COUT "Sequence "
#define FACTORIZATION_COUT "Factorization "
#define CUMULATIVELZ76_COUT "Cumulative LZ76: "
#define ASYMPTENTROPYTABLE_COUT "Asymptotic Normalized complexity table: "
#define BRUIJNENTROPYTABLE_COUT "De Bruijn normalized complexity table: "
#define RNDENTROPYTABLE_COUT "RND normalized complexity table: "
#define LZ76COMPLEXITY_COUT "LZ76: "
#define SHANNONENTROPY_COUT "Shannon Entropy Over Factor sizes: "
#define SHANNONDIVERGENCE_COUT "Shannon Divergence: "
#define MAXPHRASESIZE_COUT "MaxPhraseSize: "
#define AVERAGEPHRASESIZE_COUT "AveragePhraseSize: "
#define SIGMAAVERAGEPHRASESIZE_COUT "Standard deviation over factor size: "
#define ASYMPTENTROPY_COUT "Entropy density: "
#define SIGMAASYMPTENTROPY_COUT "Standard deviation entropy density: "
#define FITTEDENTROPY_COUT "Fitted entropy density: "
#define RNDENTROPY_COUT "RND Normalized Complexity: "
#define BRUIJNENTROPY_COUT "DBruijn Normalized Complexity: "
#define LZDISTANCE_COUT "Normalized LZ76 distance: "
#define NLZDISTANCE_COUT "LZ76 non-normalized distance: "
#define EXCESSENTROPY_COUT "Excess entropy shuffling: "
#define EXCESSENTROPYMI_COUT "Excess entropy as mutual information: "
#define REDUNDANCY_COUT "Redundancy: "
#define EXCESSENTROPYTABLE_COUT "Excess entropy table: "

#define MULTIINFORMATION_COUT "Multi information: "
#define HAMMING_COUT "Hamming compare: "
#define DLZCOMPARE_COUT "Normalized DLZ compare: "
#define NDLZCOMPARE_COUT "Non-Normalized DLZ compare: "
#define CLOSINGBRACKET " ]"
#define SPACE " "

#endif  // OUTPUTSTRINGS_H_INCLUDED
