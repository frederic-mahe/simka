/*****************************************************************************
 *   Simka: Fast kmer-based method for estimating the similarity between numerous metagenomic datasets
 *   A tool from the GATB (Genome Assembly Tool Box)
 *   Copyright (C) 2015  INRIA
 *   Authors: G.Benoit, C.Lemaitre, P.Peterlongo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include "SimkaAlgorithm.hpp"


template<size_t span>
SimkaCountProcessor<span>::SimkaCountProcessor (SimkaStatistics& stats, size_t nbBanks, const pair<size_t, size_t>& abundanceThreshold, SIMKA_SOLID_KIND solidKind, bool soliditySingle) :
_stats(stats)
{
	// We configure the vector for the N.(N+1)/2 possible pairs
	//_countTotal.resize (_nbBanks*(_nbBanks+1)/2);

	_nbBanks = nbBanks;
	_abundanceThreshold = abundanceThreshold;
	_solidKind = solidKind;
	_soliditySingle = soliditySingle;

	_localStats = new SimkaStatistics(_nbBanks);
}

template<size_t span>
SimkaCountProcessor<span>::~SimkaCountProcessor () {
	delete _localStats;
}


template<size_t span>
void SimkaCountProcessor<span>::finishClones (std::vector<ICountProcessor<span>*>& clones)
{
	for (size_t i=0; i<clones.size(); i++){

		if (SimkaCountProcessor* clone = dynamic_cast<SimkaCountProcessor*> (clones[i])){
			finishClone(clone);
			//for (size_t i=0; i<this->_countTotal.size(); i++)  { this->_countTotal[i] += clone->_countTotal[i];  }
		}
	}
}

template<size_t span>
void SimkaCountProcessor<span>::finishClone(SimkaCountProcessor<span>* clone){
	//cout << _stats << "   " << &clone->_stats << endl;
	_stats += *clone->_localStats;
}

template<size_t span>
bool SimkaCountProcessor<span>::isSolidVector(const CountVector& counts){

	//if(_solidKind == SIMKA_SOLID_KIND::RANGE){
	//}

	//bool isSolid_ = false;

	for(size_t i=0; i<counts.size(); i++){

		CountNumber abundance = counts[i];

		if(abundance == 0) continue;

		if(isSolid(abundance)){
			return true;
		}

		//nbBanks += 1;
		//if(nbBanks > 1){
		//	isSolid = true;
		//	break;
		//}
	}

	return false;

}


template<size_t span>
bool SimkaCountProcessor<span>::isSolid(CountNumber count){
	return count >= _abundanceThreshold.first && count <= _abundanceThreshold.second;
}


template<size_t span>
bool SimkaCountProcessor<span>::process (size_t partId, const Type& kmer, const CountVector& counts, CountNumber sum){

	_localStats->_nbDistinctKmers += 1;

	for(size_t i=0; i<counts.size(); i++){
		_localStats->_nbKmers += counts[i];
		_localStats->_nbKmersPerBank[i] += counts[i];
	}


	if(isSolidVector(counts))
		_localStats->_nbSolidKmers += 1;
	else
		return false;

	/*
	CountVector counts2;
	for(int i=0; i<counts.size(); i++){
		if(counts[i] < _abundanceThreshold.first)
			counts2.push_back(0);
		else
			counts2.push_back(counts[i]);
	}*/

	if(_soliditySingle){
		CountVector counts2(counts);
		for(int i=0; i<counts.size(); i++){
			if(!isSolid(counts[i]))
				counts2[i] = 0;
		}
		computeStats(counts2);
	}
	else{
		computeStats(counts);
	}


	return true;
}


template<size_t span>
void SimkaCountProcessor<span>::computeStats(const CountVector& counts){

	int nbBanksThatHaveKmer = 0;
	u_int64_t totalAbundance = 0;



	for(size_t i=0; i<counts.size(); i++){

		CountNumber abundanceI = counts[i];
		//if(abundance < _abundanceMin) continue;

		if(abundanceI){
			totalAbundance += abundanceI;
			nbBanksThatHaveKmer += 1;
			_localStats->_nbSolidDistinctKmersPerBank[i] += 1;
			_localStats->_nbSolidKmersPerBank[i] += abundanceI;
			//hasBankKmer[i] = true;

			//if(kmerInBankCoupleAbundance.first == -1)
			//	kmerInBankCoupleAbundance.first = abundance;
			//else if(kmerInBankCoupleAbundance.second == -1)
			//	kmerInBankCoupleAbundance.second = abundance;
			for(size_t j=0; j<counts.size(); j++){

				CountNumber abundanceJ = counts[j];

				if(abundanceJ){
					_localStats->_matrixNbSharedKmers[i][j] += abundanceI;
					_localStats->_matrixNbDistinctSharedKmers[i][j] += 1;
					updateBrayCurtis(i, abundanceI, j, abundanceJ);
				}

			}
		}

	}

	_localStats->_nbDistinctKmersSharedByBanksThreshold[nbBanksThatHaveKmer-1] += 1;
	_localStats->_nbKmersSharedByBanksThreshold[nbBanksThatHaveKmer-1] += totalAbundance;

	if(totalAbundance == 1){
		//if( == 1){
		_localStats->_nbErroneousKmers += 1;
		//}
	}
	//else if(nbBanksThatHaveKmer == counter.size()){
	//}


}


template<size_t span>
void SimkaCountProcessor<span>::updateBrayCurtis(int bank1, CountNumber abundance1, int bank2, CountNumber abundance2){
	_localStats->_brayCurtisNumerator[bank1][bank2] += min(abundance1, abundance2);
}



































template<size_t span>
SimkaAlgorithm<span>::SimkaAlgorithm(IProperties* options) {

	_options = options;


	_inputFilename = _options->getStr(STR_URI_INPUT);
	_outputDir = _options->get(STR_URI_OUTPUT) ? _options->getStr(STR_URI_OUTPUT) : "./";
	_kmerSize = _options->getInt(STR_KMER_SIZE);
	_abundanceThreshold.first = _options->getInt(STR_KMER_ABUNDANCE_MIN);
	_abundanceThreshold.second = _options->getInt(STR_KMER_ABUNDANCE_MAX);
	_soliditySingle = _options->get(STR_SIMKA_SOLIDITY_PER_DATASET);

	//read filter
	_maxNbReads = _options->getInt(STR_SIMKA_MAX_READS);
	_minReadSize = _options->getInt(STR_SIMKA_MIN_READ_SIZE);
	_minShannonIndex = _options->getDouble(STR_SIMKA_MIN_SHANNON_INDEX);
	_minShannonIndex = std::max(_minShannonIndex, 0.0);
	_minShannonIndex = std::min(_minShannonIndex, 2.0);

	/*
	if(_options->getInt(STR_VERBOSE) != 0){
		cout << "Filter options" << endl;
		cout << "\tMax reads per dataset:  " <<  _maxNbReads << endl;
		cout << "\tMin read size:  " <<  _minReadSize << endl;
		cout << "\tMin Shannon index:  " <<  _minShannonIndex << endl;
	}*/
	//if(_maxNbReads == 0)
	//	_maxNbReads = -1;

	//cout << _maxNbReads << endl;
	//cout << _soliditySingle << endl;
	/*
	string solidKindStr = _options->getStr(STR_SOLIDITY_KIND);
	if(solidKindStr == "range"){
		_solidKind = SIMKA_SOLID_KIND::RANGE;
	}
	else if(solidKindStr == "sum"){
		_solidKind = SIMKA_SOLID_KIND::SUM;
	}

	cout << solidKindStr << " " << solidKindStr << endl;*/
	//_kmerSize = _options->get(STR_KMER_SIZE) ? _options->getInt(STR_KMER_SIZE) : 31;
	//_abundanceMin = _options->get(STR_KMER_ABUNDANCE_MIN) ? _options->getInt(STR_KMER_ABUNDANCE_MIN) : 0;
	//_maxMemory = props->get(STR_MAX_MEMORY) ? props->getInt(STR_MAX_MEMORY) : 2000;
	//_outputTempDir = props->get(STR_URI_OUTPUT_DIR) ? props->getStr(STR_URI_OUTPUT_DIR) : System::file().getDirectory(_inputFilename);
	//_outputFilename = props->get(STR_URI_OUTPUT) ? props->getStr(STR_URI_OUTPUT) : System::file().getDirectory(_inputFilename) + "/" + System::file().getBaseName(_inputFilename) + "_output.fasta";
	//_nbCores = getInput()->getInt(STR_NB_CORES);

	//cout << "Input filename: " << _inputFilename << endl;
	//cout << "Kmer size: " << _kmerSize << endl;
	//cout << "Abundance min: " << _abundanceMin << endl;
	//cout << "Max memory: " << _maxMemory << endl;
	//cout << "Output temp dir: " << _outputTempDir << endl;
	//cout << "Output filename: " << _outputFilename << endl;


	_banksInputFilename = _inputFilename + "_dsk_dataset_temp__";


}

template<size_t span>
SimkaAlgorithm<span>::~SimkaAlgorithm() {
}


template<size_t span>
void SimkaAlgorithm<span>::execute() {

	layoutInputFilename();
	createBank();

	count();

	outputMatrix();
	outputHeatmap();

	if(_options->getInt(STR_VERBOSE) != 0){
		_stats->print();
		print();
	}

	clear();
}


template<size_t span>
void SimkaAlgorithm<span>::layoutInputFilename(){

	IFile* inputFile = System::file().newFile(_inputFilename, "rb");
	IFile* bankFile = System::file().newFile(_banksInputFilename, "wb");

	inputFile->seeko(0, SEEK_END);
	u_int64_t size = inputFile->tell();
	inputFile->seeko(0, SEEK_SET);
	char buffer2[size];
	inputFile->fread(buffer2, size, size);
	string fileContents(buffer2, size);

	string line;
	string linePart;
	vector<string> linePartList;
	stringstream fileContentsStream(fileContents);

	string bankFileContents = "";

	u_int64_t lineIndex = 0;

	while(getline(fileContentsStream, line)){

		if(line == "") continue;

		stringstream lineStream(line);
		linePartList.clear();
		//vector<string> filenames;

		while(getline(lineStream, linePart, ' ')){

			if(linePart != ""){
				linePartList.push_back(linePart);
			}
		}

		//cout << linePartList.size() << endl;
		//Bank id
		string bankId = linePartList[0];
		_bankNames.push_back(bankId);

		 //ID and one filename
		if(linePartList.size() == 2){
			bankFileContents += linePartList[1] + "\n";
			_nbReadsPerDataset.push_back(_maxNbReads);
		}
		//ID and list of filename (paired files for example)
		else{
			char buffer[200];
			snprintf(buffer,200,"%llu", lineIndex);
			string subBankFilename = _banksInputFilename + "_" + string(buffer);
			_tempFilenamesToDelete.push_back(subBankFilename);
			IFile* subBankFile = System::file().newFile(subBankFilename, "wb");
			string subBankContents = "";

			for(size_t i=1; i<linePartList.size(); i++){
				subBankContents += linePartList[i] + "\n";
			}
			subBankContents.pop_back(); // "remove last /n
			subBankFile->fwrite(subBankContents.c_str(), subBankContents.size(), 1);
			subBankFile->flush();
			delete subBankFile;

			bankFileContents += System::file().getBaseName(subBankFilename) + "\n";
			_nbReadsPerDataset.push_back(ceil(_maxNbReads / (float)(linePartList.size() - 1))); //linePartList.size() - 1 = nb sub banks
		}

		lineIndex += 1;
	}

	bankFileContents.pop_back(); // "remove last /n

	bankFile->fwrite(bankFileContents.c_str(), bankFileContents.size(), 1);

	delete inputFile;
	bankFile->flush();
	delete bankFile;

	//for(int i=0; i<_nbBanksOfDataset.size(); i++){
	//	cout << i << "   "  << _nbBanksOfDataset[i] << endl;
	//}

	if(_options->getInt(STR_VERBOSE) != 0){
		cout << "Nb input datasets: " << _bankNames.size() << endl;
	}
}


template<size_t span>
void SimkaAlgorithm<span>::createBank(){

	IBank* bank = Bank::open(_banksInputFilename);

	_nbBanks = bank->getCompositionNb();

	SimkaSequenceFilter sequenceFilter(_minReadSize, _minShannonIndex);

	_banks = new SimkaBankFiltered<SimkaSequenceFilter>(bank, sequenceFilter, _nbReadsPerDataset);

}

template<size_t span>
void SimkaAlgorithm<span>::count(){

	_stats = new SimkaStatistics(_nbBanks);

	SortingCountAlgorithm<span> sortingCount (_banks, _options);

	// We create a custom count processor and give it to the sorting count algorithm
	_processor = new SimkaCountProcessor<span> (*_stats, _nbBanks, _abundanceThreshold, _solidKind, _soliditySingle);
	_processor->use();
	sortingCount.addProcessor (_processor);

	// We launch the algorithm
	sortingCount.execute();


}

template<size_t span>
void SimkaAlgorithm<span>::outputMatrix(){

	_simkaDistance = new SimkaDistance(*_stats);

	_outputFilenameSuffix = "";

	char buffer[200];

	string strKmerSize = "_k";
	snprintf(buffer,200,"%llu",_kmerSize);
	strKmerSize += string(buffer);
	_outputFilenameSuffix += strKmerSize;

    string strAbMax = "";
    if(_abundanceThreshold.second < 1000000){
    	snprintf(buffer,200,"%llu",_abundanceThreshold.second);
    	strAbMax += "_max" + string(buffer);
    }
    _outputFilenameSuffix += strAbMax;

	string strAbMin = "_min";
	snprintf(buffer,200,"%llu",_abundanceThreshold.first);
	strAbMin += string(buffer);
	_outputFilenameSuffix += strAbMin;


	//_matDksNormFilename = "mat_dks_norm" + filenameSuffix + ".csv";
	//_matDksPercFilename = "mat_dks_asym" + filenameSuffix + ".csv";
	//_matAksNormFilename = "mat_aks_norm" + filenameSuffix + ".csv";
	//_matAksPercFilename = "mat_aks_asym" + filenameSuffix + ".csv";



    dumpMatrix("mat_presenceAbsence_asym", _simkaDistance->getMatrixDKS(SIMKA_MATRIX_TYPE::ASYMETRICAL));
    dumpMatrix("mat_presenceAbsence_norm", _simkaDistance->getMatrixDKS(SIMKA_MATRIX_TYPE::NORMALIZED));
    dumpMatrix("mat_abundance_asym", _simkaDistance->getMatrixAKS(SIMKA_MATRIX_TYPE::ASYMETRICAL));
    dumpMatrix("mat_abundance_norm", _simkaDistance->getMatrixAKS(SIMKA_MATRIX_TYPE::NORMALIZED));
    dumpMatrix("mat_brayCurtis", _simkaDistance->getMatrixBrayCurtis());

}


template<size_t span>
void SimkaAlgorithm<span>::dumpMatrix(const string& outputFilename, const vector<vector<float> >& matrix){

	char buffer[200];
	string str;

	for(int i=0; i<matrix.size(); i++){
		str += ";" + _bankNames[i];
		//str += ";" + datasetInfos[i]._name;
	}
	str += '\n';

	for(int i=0; i<matrix.size(); i++){

		str += _bankNames[i] + ";";
		//str += datasetInfos[i]._name + ";";
		for(int j=0; j<matrix.size(); j++){

			//snprintf(buffer,200,"%.2f", matrix[i][j]);
			snprintf(buffer,200,"%f", matrix[i][j]);
			str += string(buffer) + ";";

			//str += to_string(matrix[i][j]) + ";";
		}

		//matrixNormalizedStr.erase(matrixNormalizedStr.end()-1);
		str.pop_back(); //remove ; at the end of the line
		str += '\n';
	}


	gatb::core::system::IFile* file = gatb::core::system::impl::System::file().newFile(_outputDir + "/" + outputFilename + _outputFilenameSuffix + ".csv", "wb");
	file->fwrite(str.c_str(), str.size(), 1);
	file->flush();
	delete file;

}

template<size_t span>
void SimkaAlgorithm<span>::outputHeatmap(){
	__outputHeatmap("heatmap_presenceAbsence", "mat_presenceAbsence_asym", "mat_presenceAbsence_norm");
	__outputHeatmap("heatmap_abundance", "mat_abundance_asym", "mat_abundance_norm");
	__outputHeatmap("heatmap_brayCurtis", "mat_brayCurtis", "mat_brayCurtis");
}


template<size_t span>
void SimkaAlgorithm<span>::__outputHeatmap(const string& outputFilenamePrefix, const string& matrixAsymFilename, const string& matrixNormFilename){

	/*
	string filename = matrixPercFilename.substr(0, matrixPercFilename.size()-4); //remove mat extension .csv
	vector<string> linePartList;
	string outputFilename = "heatmap";
	string part;
	//string linePart;
	//vector<string> linePartList;
	stringstream stream(filename);

	while(getline(stream, part, '_')){
		//cout << part << endl;
		linePartList.push_back(part);
	}
	linePartList.erase(linePartList.begin() + 0); //Remove 'mat' prefix
	linePartList.erase(linePartList.begin() + 1); //Remove 'norm' or 'perc'

	for(size_t i=0; i<linePartList.size(); i++){
		outputFilename += "_" + linePartList[i];
	}
	outputFilename += ".png";
	*/

	string asymFilename = matrixAsymFilename + _outputFilenameSuffix + ".csv";
	string normFilename = matrixNormFilename + _outputFilenameSuffix + ".csv";
	string outputFilename = outputFilenamePrefix + _outputFilenameSuffix + ".png";


	string command = "Rscript ./Rscripts/heatmap.r " + _outputDir + "/" + asymFilename + " " + _outputDir + "/" + normFilename + " " + _outputDir + "/" + outputFilename;
	//cout << command << endl;

    try
    {
    	system(command.c_str());
    }
    catch (Exception& e)
    {
        std::cout << "EXCEPTION: " << e.getMessage() << std::endl;
        //return EXIT_FAILURE;
    }

    //if(linePartList[0] == "dks")
    //	_heatmapDksFilename = outputFilename;
    //else
    //	_heatmapAksFilename = outputFilename;

}


template<size_t span>
void SimkaAlgorithm<span>::print(){

	cout << "Output folder:   " << _outputDir << endl;

	/*
	cout << "Similarity matrix:" << endl;
	cout << "\t" << "DKS (presence/absence)" << endl;
	cout << "\t\t" << "asym: " << _outputDir + "/" + _matDksPercFilename << endl;
	cout << "\t\t" << "norm: " << _outputDir + "/" + _matDksNormFilename << endl;
	cout << "\t" << "AKS (abundance)" << endl;
	cout << "\t\t" << "asym: " << _outputDir + "/" + _matAksPercFilename << endl;
	cout << "\t\t" << "norm: " << _outputDir + "/" + _matAksNormFilename << endl;

	cout << "Heatmaps:" << endl;
	cout << "\t" << "DKS (presence/absence):" << _outputDir + "/" + _heatmapDksFilename << endl;
	cout << "\t" << "AKS (abundance):" << _outputDir + "/" + _heatmapAksFilename << endl;*/


}


template<size_t span>
void SimkaAlgorithm<span>::clear(){

	System::file().remove(_banksInputFilename);
    _processor->forget();

    for(size_t i=0; i<_tempFilenamesToDelete.size(); i++){
    	System::file().remove(_tempFilenamesToDelete[i]);
    }

    delete _stats;
    delete _simkaDistance;
	//_banks->remove();
	//delete _processor;
}
