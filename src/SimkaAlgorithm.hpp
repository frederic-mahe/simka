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

#ifndef TOOLS_SIMKA_SRC_SIMKAALGORITHM_HPP_
#define TOOLS_SIMKA_SRC_SIMKAALGORITHM_HPP_

#include <gatb/gatb_core.hpp>
#include "SimkaDistance.hpp"
#include<stdio.h>

const string STR_SIMKA_SOLIDITY_PER_DATASET = "-solidity-single";
const string STR_SIMKA_MAX_READS = "-max-reads";
const string STR_SIMKA_MIN_READ_SIZE = "-min-read-size";
const string STR_SIMKA_MIN_READ_SHANNON_INDEX = "-read-shannon-index";
const string STR_SIMKA_MIN_KMER_SHANNON_INDEX = "-kmer-shannon-index";
const string STR_KMER_PER_READ = "-kmer-per-read";

enum SIMKA_SOLID_KIND{
	RANGE,
	SUM,
};




typedef u_int16_t bankIdType;
//#define IX(rad) ((rad)+(256))
#define IX(rad) ((rad))


template<size_t span>
class SortCommand : public ICommand, public SmartPointer
{
public:
	typedef typename Kmer<span>::Type  Type;

	/** Constructor. */
	SortCommand (Type** kmervec, bankIdType** bankIdMatrix, int begin, int end, uint64_t* radix_sizes)
		: _deb(begin), _fin(end), _radix_kmers(kmervec), _bankIdMatrix(bankIdMatrix), _radix_sizes(radix_sizes) {}

	/** */
	void execute ()
	{
		vector<size_t> idx;
		vector<Tmp>    tmp;

		for (int ii=_deb; ii <=_fin; ii++)
		{
			if (_radix_sizes[ii] > 0)
			{
				/** Shortcuts. */
				Type* kmers = _radix_kmers  [ii];

				//if (_bankIdMatrix)
				//{
					/** NOT OPTIMAL AT ALL... in particular we have to use 'idx' and 'tmp' vectors
					 * which may use (a lot of ?) memory. */

					/** Shortcut. */
					bankIdType* banksId = _bankIdMatrix [ii];

					/** NOTE: we sort the indexes, not the items. */
					idx.resize (_radix_sizes[ii]);
					for (size_t i=0; i<idx.size(); i++)  { idx[i]=i; }

					std::sort (idx.begin(), idx.end(), Cmp(kmers));

					/** Now, we have to reorder the two provided vectors with the same order. */
					tmp.resize (idx.size());
					for (size_t i=0; i<idx.size(); i++)
					{
						tmp[i].kmer = kmers  [idx[i]];
						tmp[i].id   = banksId[idx[i]];
					}
					for (size_t i=0; i<idx.size(); i++)
					{
						kmers  [i] = tmp[i].kmer;
						banksId[i] = tmp[i].id;
					}
				//}
				//else
			//{
			//	std::sort (&kmers[0] , &kmers[ _radix_sizes[ii]]);
			//	}
			}
		}
	}

private :

	struct Tmp { Type kmer;  bankIdType id;};

	struct Cmp
	{
		Type* _kmers;
		Cmp (Type* kmers) : _kmers(kmers) {}
		bool operator() (size_t a, size_t b)  { return _kmers[a] < _kmers[b]; }
	};

	int        _deb;
	int        _fin;
	Type**     _radix_kmers;
	bankIdType** _bankIdMatrix;
	uint64_t*  _radix_sizes;
};




template<size_t span>
class PartitionsCommand : public gatb::core::tools::dp::ICommand, public SmartPointer
{
public:

    /** Shortcut. */
    typedef typename Kmer<span>::Type           Type;
    typedef typename Kmer<span>::Count          Count;
    typedef ICountProcessor<span> CountProcessor;

    /** Constructor. */
    PartitionsCommand (
        gatb::core::tools::collections::Iterable<Type>& partition,
        CountProcessor*                                 processor,
        size_t                                          cacheSize,
		//gatb::core::tools::dp::IteratorListener*        progress,
		//tools::misc::impl::TimeInfo&                    timeInfo,
        //PartiInfo<5>&                                   pInfo,
		int                                             passi,
		int                                             parti,
		size_t                                          nbCores,
		size_t                                          kmerSize,
		gatb::core::tools::misc::impl::MemAllocator&    pool
    )
    :
      _partition(partition),
      //_progress(progress),
	  //_pInfo(pInfo),
	  _pass_num(passi),
      _parti_num(parti),
      _nbCores(nbCores),
      _kmerSize(kmerSize),
      _cacheSize(cacheSize),
      _pool(pool),
	  //_globalTimeInfo(timeInfo),
      _processor(0)
	{
		setProcessor      (processor);
	}

    /** Destructor. */
    ~PartitionsCommand(){

    }

    /** Get the class name (for statistics). */
    virtual const char* getName() const = 0;

protected:
    gatb::core::tools::collections::Iterable<Type>&         _partition;
    //gatb::core::tools::dp::IteratorListener*                _progress;
    //PartiInfo<5>&                                           _pInfo;
    int                                                     _pass_num;
	int                                                     _parti_num;
    size_t                                                  _nbCores;
	size_t                                                  _kmerSize;
    size_t                                                  _cacheSize;
	gatb::core::tools::misc::impl::MemAllocator&            _pool;

	//void insert (const Type& kmer, const CounterBuilder& count);

	//tools::misc::impl::TimeInfo& _globalTimeInfo;
	//tools::misc::impl::TimeInfo  _timeInfo;

    CountProcessor* _processor;
    void setProcessor (CountProcessor* processor)  { SP_SETATTR(processor); }
};


template<size_t span>
class KReader
{
	typedef typename Kmer<span>::Type  Type;
public:

	void operator() (Type& elem)
	{

		//uint64_t idx;
		u_int8_t rid = getHeavyWeight(elem).getVal();//getHeavyWeight(elem.getVal());

		//u_int64_t idx = _r_idx +  IX(rid); //__sync_fetch_and_add( _r_idx +  IX(rid) ,1);
		u_int64_t idx = __sync_fetch_and_add( _r_idx +  IX(rid) ,1); // si le sync fetch est couteux, faire un mini buffer par thread

		//cout << idx << endl;
		//_radix_kmers [IX(rid)][ idx] = kinsert << ((4-kx_size)*2);  //[kx_size][rid]
		_radix_kmers[IX(rid)][idx] = elem;  //[kx_size][rid]
		_bankIdMatrix[IX(rid)][idx] = _bankId;

		//cout << elem.getVal() << endl;
		/*
		//reading elems by pairs
		if(_first)
		{
			_superk = elem;
			_first = false;
		}
		else
		{
			_seedk = elem;

			Type compactedK;

			compactedK =  _superk;
			u_int8_t nbK = (compactedK >> _shift_val).getVal()  & 255; // 8 bits poids fort = cpt
			u_int8_t rem = nbK;

			Type temp = _seedk;
			Type rev_temp = revcomp(temp,_kmerSize);
			Type newnt ;
			Type mink, prev_mink;
			uint64_t idx;

			bool prev_which =  (temp < rev_temp );
			int kx_size = -1; //next loop start at ii=0, first kmer will put it at 0
			Type radix_kxmer_forward =  (temp & _mask_radix) >> ((_kmerSize - 4)*2);
			Type  first_revk, kinsert,radix_kxmer;

			if(!prev_which) first_revk = rev_temp;

			u_int8_t rid;

			for (int ii=0; ii< nbK; ii++,rem--)
			{
				bool which =  (temp < rev_temp );
				mink = which ? temp : rev_temp;

				if (which != prev_which || kx_size >= _kx) // kxmer_size = 1
				{
					//output kxmer size kx_size,radix_kxmer
					//kx mer is composed of superKp[ii-1] superKp[ii-2] .. superKp[ii-n] with nb elems  n  == kxmer_size +1  (un seul kmer ==k+0)

					if(prev_which)
					{
						radix_kxmer = radix_kxmer_forward;
						kinsert = prev_mink;
					}
					else // si revcomp, le radix du kxmer est le debut du dernier kmer
					{
						//previous mink
						radix_kxmer =  (prev_mink & _mask_radix) >> _shift_radix;
						kinsert = first_revk;
					}

					//record kxmer
					rid = radix_kxmer.getVal();
					//idx = _r_idx[IX(kx_size,rid)]++;
					idx = __sync_fetch_and_add( _r_idx +  IX(rid) ,1); // si le sync fetch est couteux, faire un mini buffer par thread

					_radix_kmers [IX(rid)][ idx] = kinsert << ((4-kx_size)*2);  //[kx_size][rid]
					if (_bankIdMatrix)  { _bankIdMatrix[IX(rid)][ idx] = _bankId; }

					radix_kxmer_forward =  (mink & _mask_radix) >> _shift_radix;
					kx_size =0;

					if(!which) first_revk = rev_temp;
				}
				else
				{
					kx_size++;
				}

				prev_which = which ;
				prev_mink = mink;

				if(rem < 2) break; //no more kmers in this superkmer, the last one has just been eaten
				newnt =  ( _superk >> ( 2*(rem-2)) ) & 3 ;

				temp = ((temp << 2 ) |  newnt   ) & _kmerMask;
				newnt =  Type(comp_NT[newnt.getVal()]) ;
				rev_temp = ((rev_temp >> 2 ) |  (newnt << _shift) ) & _kmerMask;
			}

			//record last kxmer prev_mink et monk ?
			if(prev_which)
			{
				radix_kxmer = radix_kxmer_forward;
				kinsert = prev_mink;
			}
			else // si revcomp, le radix du kxmer est le debut du dernier kmer
			{
				//previous mink
				radix_kxmer =  (prev_mink & _mask_radix) >> _shift_radix;
				kinsert = first_revk;
			}

			//record kxmer
			rid = radix_kxmer.getVal();
			//idx = _r_idx[IX(kx_size,rid)]++;
			idx = __sync_fetch_and_add( _r_idx +  IX(kx_size,rid) ,1); // si le sync fetch est couteux, faire un mini buffer par thread

			_radix_kmers [IX(kx_size,rid)][ idx] = kinsert << ((4-kx_size)*2);   // [kx_size][rid]
			if (_bankIdMatrix)  { _bankIdMatrix[IX(kx_size,rid)][ idx] = _bankId; }

			_first = true;
		}*/
	}

	KReader (size_t kmerSize,  uint64_t * r_idx, Type** radix_kmers, bankIdType** bankIdMatrix, bankIdType bankId=0)
	: _kmerSize (kmerSize), _kx(4), _radix_kmers(radix_kmers), _bankIdMatrix(bankIdMatrix), _r_idx (r_idx), _first(true), _bankId(bankId)
	 {
		//Type un = 1;
		 //_kmerMask    = (un << (kmerSize*2)) - un;
		 _mask_radix  = Type((int64_t) 255);
		 _mask_radix  = _mask_radix << ((_kmerSize - 4)*2);
		 //_shift       = 2*(kmerSize-1);
		 //_shift_val   = un.getSize() -8;
		 //_shift_radix = ((kmerSize - 4)*2); // radix is 4 nt long
	}

private :

    Type getHeavyWeight (const Type& kmer) const  {  return (kmer & this->_mask_radix) >> ((this->_kmerSize - 4)*2);  }

	size_t _kmerSize;
	size_t _shift ;
	size_t _shift_val ;
	size_t _shift_radix ;
	int    _kx;
	Type** _radix_kmers;
	bankIdType** _bankIdMatrix;
	uint64_t* _r_idx ;
	bool _first;
	Type _superk, _seedk;
	Type _radix, _mask_radix ;
	Type _kmerMask;
	bankIdType _bankId;
};








template<size_t span>
class KxmerPointer
{
public:
    typedef typename Kmer<span>::Type  Type;

    //on lui passe le vector dun kxmer //std::vector<vector<Type> >  & kmervec
    KxmerPointer (
        Type**      kmervec,
        int         prefix_size,
        int         x_size,
        int         min_radix,
        int         max_radix,
        int         kmerSize,
        uint64_t*   radix_sizes,
        bankIdType**  bankIdMatrix
    )
        : _kxmers(kmervec), _bankIdMatrix(0), _radix_sizes(radix_sizes), _cur_idx(-1),
          _low_radix(min_radix),_high_radix(max_radix),
          _prefix_size(prefix_size), _kmerSize(kmerSize),  _x_size(x_size)
    {
        _idx_radix = min_radix;
        //Type un = 1;
        //_kmerMask = (un << (_kmerSize*2)) - un;

        //_shift_size = ( (4 - _prefix_size) *2) ;
        //_radixMask = Type(_idx_radix) ;
        //_radixMask = _radixMask << ((_kmerSize-4)*2);
        //_radixMask = _radixMask  << (2*_prefix_size)  ;

        _bankIdMatrix = bankIdMatrix + IX(0);
    }

    /** */
    inline bool next ()
    {
        _cur_idx++;

        // go to next non empty radix
        while(_idx_radix<= _high_radix && (uint64_t)_cur_idx >=   _radix_sizes[_idx_radix])
        {
            _idx_radix++;
            _cur_idx = 0;
            //update radix mask does not happen often
            //_radixMask = Type(_idx_radix) ;
            //_radixMask = _radixMask << ((_kmerSize-4)*2);
            //_radixMask = _radixMask  << (2*_prefix_size)  ;
        }

        return (_idx_radix <= _high_radix);
    }

    /** */
    //inline Type    value   () const  {  return ( ((_kxmers[_idx_radix][_cur_idx]) >> _shift_size)  |  _radixMask  ) & _kmerMask ;  }
    inline Type    value   () const  {  return _kxmers[_idx_radix][_cur_idx];  }

    /** */
    inline bankIdType getBankId () const  {
    	//cout << _idx_radix << " " << _cur_idx << endl;
    	return  _bankIdMatrix [_idx_radix][_cur_idx];  }

private :

    Type**      _kxmers;
    bankIdType**  _bankIdMatrix;
    uint64_t*   _radix_sizes;
    int64_t     _cur_idx;
    Type        _cur_radix;
    Type        _kmerMask;
    Type        _radixMask;
    int         _idx_radix;
    int         _low_radix;
    int         _high_radix;
    int         _shift_size;
    int         _prefix_size;
    int         _kmerSize;
    int         _x_size; //x size of the _kxmersarray
};



class CounterBuilder
{
public:

    /** Constructor.
     * \param[in] nbBanks : number of banks parsed during kmer counting.
     */
    CounterBuilder (size_t nbBanks=1)  :  _abundancePerBank(nbBanks)  {}

    /** Get the number of banks.
     * \return the number of banks. */
    size_t size() const  { return _abundancePerBank.size(); }

    /** Initialization of the counting for the current kmer. This method should be called
     * when a kmer is seen for the first time.
     * \param[in] idxBank : bank index where the new current kmer has been found. */
    void init (size_t idxBank=0)
    {
        for (size_t k=0; k<_abundancePerBank.size(); k++)  { _abundancePerBank[k]=0; }
        _abundancePerBank [idxBank]= 1;
    }

    /** Increase the abundance of the current kmer for the provided bank index.
     * \param[in] idxBank : index of the bank */
    void increase (size_t idxBank=0)  {  _abundancePerBank [idxBank] ++;  }

    /** Set the abundance of the current kmer for the provided bank index.
     * \param[in] idxBank : index of the bank */
    void set (CountNumber val, size_t idxBank=0)  {  _abundancePerBank [idxBank] = val;  }

    /** Get the abundance of the current kmer for the provided bank index.
     * \param[in] idxBank : index of the bank
     * \return the abundance of the current kmer for the given bank. */
    CountNumber operator[] (size_t idxBank) const  { return _abundancePerBank[idxBank]; }

    /** */
    const CountVector& get () const { return _abundancePerBank; }

private:
    CountVector _abundancePerBank;
};


/********************************************************************************/
/** */
template<size_t span>
class PartitionCommand : public PartitionsCommand<span>
{
public:

    /** Shortcut. */ /* R: don't know how to avoid this code duplication */
    typedef typename Kmer<span>::Type           Type;
    typedef typename Kmer<span>::Count          Count;
    typedef ICountProcessor<span> CountProcessor;

    static const size_t KX = 1;

private:
    //used for the priority queue
    //typedef std::pair<int, Type> kxp; //id pointer in vec_pointer , value
    //struct kxpcomp { bool operator() (kxp l,kxp r) { return ((r.second) < (l.second)); } } ;

public:
    /** Constructor. */
    PartitionCommand (
        gatb::core::tools::collections::Iterable<Type>& partition,
        CountProcessor*                                 processor,
        size_t                                          cacheSize,
		//gatb::core::tools::dp::IteratorListener*        progress,
		//tools::misc::impl::TimeInfo&                    timeInfo,
		//PartiInfo<5>&                                   pInfo,
		int                                             passi,
        int                                             parti,
        size_t                                          nbCores,
        size_t                                          kmerSize,
        gatb::core::tools::misc::impl::MemAllocator&    pool,
        std::vector<size_t>&                            offsets,
		vector<u_int64_t>& nbKmerPerPartitions,
		vector<vector<u_int64_t> >& nbk_per_radix_per_part
    ):
    	PartitionsCommand<span>(partition, processor, cacheSize, passi, parti, nbCores, kmerSize, pool),
        _radix_kmers (0), _bankIdMatrix(0), _radix_sizes(0), _r_idx(0), _nbItemsPerBankPerPart(offsets), _nbKmerPerPartitions(nbKmerPerPartitions),
		_nbk_per_radix_per_part(nbk_per_radix_per_part)
	{
		_dispatcher = new Dispatcher (this->_nbCores);
		//_dispatcher = new Dispatcher (1);
	}

    /** Destructor. */
    ~PartitionCommand (){
        if (_dispatcher)  { delete _dispatcher; }
    }

    /** Get the class name (for statistics). */
    const char* getName() const { return "vector"; }



    void execute ()
    {
        this->_processor->beginPart (this->_pass_num, this->_parti_num, this->_cacheSize, this->getName());

        /** We check that we got something. */
        if (this->_partition.getNbItems() == 0)  {  return;  }

        /** We configure tables. */
        _radix_kmers  = (Type**)     MALLOC (256*(KX+1)*sizeof(Type*)); //make the first dims static ?  5*256
        _radix_sizes  = (uint64_t*)  MALLOC (256*(KX+1)*sizeof(uint64_t));
        _r_idx        = (uint64_t*)  CALLOC (256*(KX+1),sizeof(uint64_t));

        _bankIdMatrix = (bankIdType**) MALLOC (256*(KX+1)*sizeof(bankIdType*));
        /** We need extra information for kmers counting in case of several input banks. */
        //if (_nbItemsPerBankPerPart.size() > 1) { _bankIdMatrix = (bankIdType**) MALLOC (256*(KX+1)*sizeof(bankIdType*)); }
        //else                                   { _bankIdMatrix = 0; }

        /** We have 3 phases here: read, sort and dump. */
        executeRead ();
        executeSort ();
        executeDump ();

        /** We cleanup tables. */
        FREE (_radix_sizes) ;
        FREE (_radix_kmers);
        FREE (_r_idx);
        FREE (_bankIdMatrix);
        //if (_bankIdMatrix)  { FREE (_bankIdMatrix); }

        /** We update the progress bar. */
        //this->_progress->inc (this->_pInfo.getNbKmer(this->_parti_num) );

        //this->_processor->endPart (this->_pass_num, this->_parti_num);
    };


    void executeRead ()
    {


        //uint64_t sum_nbxmer =0;

        //int totalkmer = 0;

        {

            LocalSynchronizer synchro (this->_pool.getSynchro());

            this->_pool.align (16);

            //for (size_t xx=0; xx< (KX+1); xx++)
            //{
                for (int ii=0; ii< 256; ii++)
                {
                    //size_t nbKmers = _nbKmerPerPartitions(this->_parti_num,ii,xx);
                    u_int64_t nbKmers = _nbk_per_radix_per_part[ii][this->_parti_num];
                    //totalkmer += nbKmers;
                    //if(nbKmers > 1)
                    //cout << nbKmers << endl;
                    //size_t nbKmers = this->_pInfo.getNbKmer(this->_parti_num,ii,xx);

                    //use memory pool here to avoid memory fragmentation
                    _radix_kmers  [IX(ii)] = (Type*)     this->_pool.pool_malloc (nbKmers * sizeof(Type),     "kmers alloc");
                    _radix_sizes  [IX(ii)] = nbKmers;

                    //sum_nbxmer +=  nbKmers;
                }
            //}

            //if (_bankIdMatrix)
                //{
				for (int ii=0; ii< 256; ii++)
				{
					//size_t nbKmers = this->_pInfo.getNbKmer(this->_parti_num,ii,xx);
					u_int64_t nbKmers = _nbk_per_radix_per_part[ii][this->_parti_num];
					_bankIdMatrix [IX(ii)] = (bankIdType*) this->_pool.pool_malloc (nbKmers * sizeof(bankIdType), "bank ids alloc");
				}
				//}
        }

        //cout << totalkmer << endl;


        //if (_bankIdMatrix)
        //{
            Iterator<Type>* itGlobal = this->_partition.iterator();
            LOCAL (itGlobal);

            for (size_t b=0; b<_nbItemsPerBankPerPart.size(); b++)
            {
                Iterator<Type>* itLocal = new TruncateIterator<Type> (*itGlobal, _nbItemsPerBankPerPart[b], b==0 ? true : false);
                LOCAL (itLocal);

                _dispatcher->iterate (itLocal, KReader<span>  (this->_kmerSize, _r_idx, _radix_kmers, _bankIdMatrix, b), 10000); //must be even , reading by pairs
            }

            if (itGlobal->isDone() == false)  { throw Exception ("PartitionsByVectorCommand: iteration should be finished"); }
        //}
        //else
        //{
           	   //_dispatcher->iterate (this->_partition.iterator(), SuperKReader<span>  (this->_kmerSize, _r_idx, _radix_kmers, 0, 0), 10000); //must be even , reading by pairs
        //}
    }



    void executeSort ()
    {
        //TIME_INFO (this->_timeInfo, "2.sort");

        vector<ICommand*> cmds;

        int nwork = 256 / this->_nbCores;

        //for (size_t xx=0; xx < (KX+1); xx++)
        //{
            cmds.clear();

            //fill cmd work vector
            for (size_t tid=0; tid < this->_nbCores; tid++)
            {
                int deb = 0 + tid * nwork;
                int fin = (tid+1) * nwork -1; // thread will do inclusive range [begin -- end ]
                if(tid == this->_nbCores-1)  { fin = 255; }

                // mettre dans le  SortCommand le master radix_kmers et range a traiter
                cmds.push_back (new SortCommand<span> (
                    _radix_kmers+ IX(0),
                    _bankIdMatrix+ IX(0),
                    deb, fin,
                    _radix_sizes + IX(0)
                ));
            }

            _dispatcher->dispatchCommands (cmds, 0);
        //}
    }


    void executeDump ()
    {

        CounterBuilder solidCounter (_nbItemsPerBankPerPart.size());

    	//cout << "\n\n\n" << endl;
    	KxmerPointer<span>* pointer = new KxmerPointer<span> (_radix_kmers+ IX(0) ,0,0,0,255,this->_kmerSize, _radix_sizes + IX(0), _bankIdMatrix);

        Type previous_kmer = 0;
        //Type currentKmer;
        pointer->next();
        previous_kmer = pointer->value();
        solidCounter.init (pointer->getBankId());

    	while(pointer->next()){
    		Type currentKmer = pointer->value();

    		if(previous_kmer != currentKmer){
    			//cout << "\t" << pointer->getBankId() << endl;
    			this->_processor->process (this->_parti_num, previous_kmer, solidCounter.get());
    			previous_kmer = currentKmer;
    	        solidCounter.init (pointer->getBankId());
                //solidCounter.increase (pointer->getBankId());
    		}
    		else{
                solidCounter.increase (pointer->getBankId());
    		}

    		//cout << pointer->value().toString(this->_kmerSize) << endl;
    	}

    	/*
		Type currentKmer = pointer->value();

		if(previous_kmer != currentKmer){
			//cout << "\t" << pointer->getBankId() << endl;
			this->_processor->process (this->_parti_num, previous_kmer, solidCounter.get());
			previous_kmer = currentKmer;
	        solidCounter.init (pointer->getBankId());
            //solidCounter.increase (pointer->getBankId());
		}
		else{
            solidCounter.increase (pointer->getBankId());
		}*/

		this->_processor->process (this->_parti_num, previous_kmer, solidCounter.get());
    	//while(pointer->)
    	/*
        TIME_INFO (this->_timeInfo, "3.dump");

        int nbkxpointers = 453; //6 for k1 mer, 27 for k2mer, 112 for k3mer  453 for k4mer
        vector< KxmerPointer<span>*> vec_pointer (nbkxpointers);
        int best_p;

        std::priority_queue< kxp, std::vector<kxp>,kxpcomp > pq;

        CounterBuilder solidCounter (_nbItemsPerBankPerPart.size());

        Type previous_kmer ;

        //init the pointers to the 6 arrays
        int pidx =0;



        //fill the  priority queue with the first elems
        for (int ii=0; ii<nbkxpointers; ii++)
        {
            if(vec_pointer[ii]->next())  {  pq.push(kxp(ii,vec_pointer[ii]->value()));  }
        }

        if (pq.size() != 0) // everything empty, no kmer at all
        {
            //get first pointer
            best_p = pq.top().first ; pq.pop();

            previous_kmer = vec_pointer[best_p]->value();

            solidCounter.init (vec_pointer[best_p]->getBankId());

            //merge-scan all 'virtual' arrays and output counts
            while (1)
            {
                //go forward in this array or in new array of reaches end of this one
                if (! vec_pointer[best_p]->next())
                {
                    //reaches end of one array
                    if(pq.size() == 0) break; //everything done

                    //otherwise get new best
                    best_p = pq.top().first ; pq.pop();
                }

                if (vec_pointer[best_p]->value() != previous_kmer )
                {
                    //if diff, changes to new array, get new min pointer
                    pq.push(kxp(best_p,vec_pointer[best_p]->value())); //push new val of this pointer in pq, will be counted later

                    best_p = pq.top().first ; pq.pop();

                    //if new best is diff, this is the end of this kmer
                    if(vec_pointer[best_p]->value()!=previous_kmer )
                    {
                        this->insert (previous_kmer, solidCounter);

                        solidCounter.init (vec_pointer[best_p]->getBankId());
                        previous_kmer = vec_pointer[best_p]->value();
                    }
                    else
                    {
                        solidCounter.increase (vec_pointer[best_p]->getBankId());
                    }
                }
                else
                {
                    solidCounter.increase (vec_pointer[best_p]->getBankId());
                }
            }

            //last elem
            this->insert (previous_kmer, solidCounter);
        }

        for (int ii=0; ii<nbkxpointers; ii++)  {  delete vec_pointer[ii];  }*/
    }


private:

    Type**     	       _radix_kmers;
    bankIdType** _bankIdMatrix;
	uint64_t*          _radix_sizes;
	uint64_t*          _r_idx;

    IDispatcher* _dispatcher;

    //void executeRead   ();
    //void executeSort   ();
    //void executeDump   ();

    std::vector<size_t> _nbItemsPerBankPerPart;
    vector<u_int64_t>& _nbKmerPerPartitions;
	vector<vector<u_int64_t> >& _nbk_per_radix_per_part;
};




template<size_t span>
class FillPartitions
{
public:

    typedef typename Kmer<span>::Type                                       Type;
    typedef typename Kmer<span>::ModelCanonical                             ModelCanonical;
    typedef typename ModelCanonical::Kmer                                   KmerType;

    FillPartitions (size_t nbMinimizers, size_t nbPartitions, size_t kmerSize, u_int64_t estimateNbSequences, Partition<Type>* partition, vector<vector<u_int64_t> >& nbk_per_radix_per_part, double minKmerShannonIndex) :
    	_nbk_per_radix_per_part(nbk_per_radix_per_part), _nbPartitions(nbPartitions), _nbMinimizers(nbMinimizers), _kmerSize(kmerSize), _model(_kmerSize), _partition (*partition,1<<12,0), _minKmerShannonIndex(minKmerShannonIndex)
    {
        _mask_radix = (int64_t) 255 ;
        _mask_radix = _mask_radix << ((this->_kmerSize - 4)*2); //get first 4 nt  of the kmers (heavy weight)
    	//u_int64_t nbEntries = estimateNbSequences * _nbMinimizers;
    	//u_int64_t nbCreated = 0;
    	//_counter = new Hash16<Type, u_int8_t>(nbEntries, &nbCreated);

        _nbk_per_radix_per_part_local.resize(256);
        for(size_t ii=0; ii<256; ii++)
    	{
    		_nbk_per_radix_per_part_local[ii].resize(_nbPartitions, 0);
    	}

    }


    ~FillPartitions ()
    {
    	//cout << "lala" << endl;

        for(size_t i=0; i<256; i++)
        	for(size_t j=0; j<_nbPartitions; j++)
        		_nbk_per_radix_per_part[i][j] += _nbk_per_radix_per_part_local[i][j];
    }

    /*:   //Sequence2SuperKmer<span> (model, nbPasses, currentPass, nbPartitions, progress, bankStats),
        //_kx(4),
        //_extern_pInfo(pInfo) , _local_pInfo(nbPartitions,model.getMmersModel().getKmerSize()),
        _repartition (repartition), _partition (*partition,1<<12,0)
    {
        _mask_radix = (int64_t) 255 ;
        _mask_radix = _mask_radix << ((this->_kmersize - 4)*2); //get first 4 nt  of the kmers (heavy weight)
    }*/



    /** Shared resources (must support concurrent accesses). */

    Type getHeavyWeight (const Type& kmer) const  {  return (kmer & this->_mask_radix) >> ((this->_kmerSize - 4)*2);  }


    void operator() (Sequence& sequence){

    	//cout << sequence.toString() << endl;

    	if(sequence.getDataSize() < _kmerSize) return;

		std::vector<KmerType> kmers;
		_model.build(sequence.getData(), kmers);


		if(_minKmerShannonIndex != 0){
			for(size_t i=0; i<kmers.size();){

				float shannonIndex = getShannonIndex(kmers[i].value());

				if(shannonIndex < _minKmerShannonIndex){
					kmers.erase(kmers.begin() + i);
				}
				else{
					i += _kmerSize/3;
				}
			}
		}

		if(kmers.size() <= 0) return;

		size_t nbMinimizer = min(_nbMinimizers, kmers.size()) ;
		std::vector<KmerType> minimizers;
		minHash(nbMinimizer, kmers, minimizers);



		for(size_t i=0; i<minimizers.size(); i++){

			KmerType& minimizer = minimizers[i];
			size_t p = oahash(minimizer.value()) % _nbPartitions;
			this->_partition[p].insert(minimizer.value());

			incKmer_and_rad(p, getHeavyWeight(minimizer.value()).getVal());
			//cout << getHeavyWeight(minimizer.value()).getVal() << endl;
			//_counter->insert(minimizer.value());
			/*
	        u_int64_t h = minimizer.minimizer().value().getVal();
			superKmer.minimizer    = h;
	        superKmer.addKmer(minimizer);
	        processSuperkmer (superKmer);
			superKmer.reset();*/
		}

		/*
        if(prev_which)
        {
            radix_kxmer = radix_kxmer_forward;
        }
        else // si revcomp, le radix du kxmer est le debut du dernier kmer
        {
            radix_kxmer = getHeavyWeight (superKmer[ii-1].value());
        }

        this->_local_pInfo.incKmer_and_rad (p, radix_kxmer.getVal(), kx_size); //nb of superkmer per x per parti per radix
        */
		            /** We save the superkmer into the right partition. */
		            //superKmer.save (this->_partition[p]);


    }

    inline void incKmer_and_rad(int numpart, int radix,  u_int64_t val=1) //appele ds vrai loop
    {
        //_nb_kxmers_per_parti[numpart] += val;            // now used to store number of kxmers per parti
        //_nb_kmers_per_parti [numpart] += (val * (x+1));  // number of  'real' kmers
        _nbk_per_radix_per_part_local[radix][numpart] += val; // contains number of kx mer per part per radix per x
    }

	//bool isShannonIndexValid(const Type&  kmer){
    //	float shannon = getShannonIndex(kmer);
		//if(shannon < 1){
		//	cout << kmer.toString(_kmerSize) << endl;
		//}
		//if(_minShannonIndex == 0) return true;
    //	return shannon > 1.8;
    //}

	float getShannonIndex(const Type&  kmer){
		float index = 0;
		//float freq [5];

		vector<float> _freqs(4, 0);

		//char* seqStr = seq.getDataBuffer();

        for (size_t i=0; i<_kmerSize; i++){
        	_freqs[kmer[i]] += 1.0;
        	//seq[sizeKmer-i-1] = bin2NT [(*this)[i]];
        }

		// Frequency of each letter (A, C, G, T or N)
		//for(size_t i=0; i < seq.size(); i++)
		//	_freqs[nt2binTab[(unsigned char)seq[i]]] += 1.0;

		// Shannon index calculation
		for (size_t i=0; i<_freqs.size(); i++){
			_freqs[i] /= (float) _kmerSize;
			if (_freqs[i] != 0)
				index += _freqs[i] * log (_freqs[i]) / log(2);
		}
		return abs(index);

	}

private:

    vector<vector<u_int64_t> >& _nbk_per_radix_per_part;//number of kxmer per parti per rad
    vector<vector<u_int64_t> > _nbk_per_radix_per_part_local;

    size_t _nbPartitions;
    size_t _nbMinimizers;
    size_t _kmerSize;
    ModelCanonical _model;
    //Hash16<Type, u_int8_t>* _counter;

    /** Shared resources (must support concurrent accesses). */
    PartitionCache <Type> _partition;
    double _minKmerShannonIndex;
    //PartitionCacheType <Type> _partition;

    //size_t        _kx;
    //PartiInfo<5>& _extern_pInfo;
    //PartiInfo<5>  _local_pInfo;
	Type          _mask_radix;
	//Repartitor&   _repartition;
    /*
    uint64_t xorshift64(u_int64_t x) {
    	x ^= x >> 12;
    	x ^= x << 25;
    	x ^= x >> 27;
    	return x * UINT64_C(2685821657736338717);
    }*/


    void minHash(int minimizerCount, std::vector<KmerType>& kmers, std::vector<KmerType>& result){
    	result.resize(minimizerCount);
    	std::vector<u_int64_t> sketch(minimizerCount);
    	uint64_t hashValue;

    	KmerType kmer = kmers[0];
    	hashValue = oahash(kmer.value());

    	for(int j=0; j < minimizerCount; ++j){
    		sketch[j] = hashValue;
    		result[j] = kmer;
    		hashValue = oahash(kmer.value());
    	}

    	for(KmerType& kmer : kmers){
    		hashValue = oahash(kmer.value());

    		for(int j = 0; j < minimizerCount; ++j){
    			if(hashValue < sketch[j]){
    				sketch[j] = hashValue;
    				result[j] = kmer;
    			}
    			hashValue = oahash(kmer.value());
    		}
    	}

    }

};



















/*********************************************************************
* ** SimkaCountProcessor
*********************************************************************/
template<size_t span>
class SimkaCountProcessor : public CountProcessorAbstract<span>{

public:

    typedef typename Kmer<span>::Type  Type;
    //typedef typename Kmer<span>::Count Count;

	SimkaCountProcessor(SimkaStatistics& stats, size_t nbBanks, const pair<size_t, size_t>& abundanceThreshold, SIMKA_SOLID_KIND solidKind, bool soliditySingle);
	~SimkaCountProcessor();
    CountProcessorAbstract<span>* clone ()  {  return new SimkaCountProcessor (_stats, _nbBanks, _abundanceThreshold, _solidKind, _soliditySingle);  }
	//CountProcessorAbstract<span>* clone ();
	void finishClones (vector<ICountProcessor<span>*>& clones);
	void finishClone(SimkaCountProcessor<span>* clone);
	virtual bool process (size_t partId, const typename Kmer<span>::Type& kmer, const CountVector& count, CountNumber sum);

	void computeStats(const CountVector& counts);
	void updateBrayCurtis(int bank1, CountNumber abundance1, int bank2, CountNumber abundance2);
	void updateKullbackLeibler(int bank1, CountNumber abundance1, int bank2, CountNumber abundance2);

	bool isSolidVector(const CountVector& counts);
	bool isSolid(CountNumber count);


private:

    size_t         _nbBanks;
	pair<size_t, size_t> _abundanceThreshold;
    SIMKA_SOLID_KIND _solidKind;
    bool _soliditySingle;
    //vector<size_t> _countTotal;

	//u_int64_t _nbBanks;
    SimkaStatistics* _localStats;
    SimkaStatistics& _stats;
    u_int64_t _totalAbundance;
};

struct SimkaSequenceFilter
{
	//u_int64_t _maxNbReads;
	//u_int64_t _maxNbReadsPerBank;
	//u_int64_t _nbReadProcessed;
	//int* _bankIndex;
	//int* _datasetIndex;

	SimkaSequenceFilter(size_t minReadSize, double minShannonIndex){

		_minReadSize = minReadSize;
		_minShannonIndex = minShannonIndex;
	}

	bool operator() (Sequence& seq){

		if(!isReadSizeValid(seq))
			return false;

		if(!isShannonIndexValid(seq))
			return false;

		return true;
	}

	bool isReadSizeValid(Sequence& seq){
		if(_minReadSize == 0) return true;
		return seq.getDataSize() >= _minReadSize;
	}

	bool isShannonIndexValid(Sequence& seq){
		if(_minShannonIndex == 0) return true;
		return getShannonIndex(seq) >= _minShannonIndex;
	}

	float getShannonIndex(Sequence& seq){
		float index = 0;
		//float freq [5];

		vector<float> _freqs(5, 0);

		char* seqStr = seq.getDataBuffer();

		// Frequency of each letter (A, C, G, T or N)
		for(size_t i=0; i < seq.getDataSize(); i++)
			_freqs[nt2binTab[(unsigned char)seqStr[i]]] += 1.0;

		// Shannon index calculation
		for (size_t i=0; i<_freqs.size(); i++){
			_freqs[i] /= (float) seq.getDataSize();
			if (_freqs[i] != 0)
				index += _freqs[i] * log (_freqs[i]) / log(2);
		}
		return abs(index);

	}

	size_t _minReadSize;
	double _minShannonIndex;
	const char nt2binTab[128] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0, 0, //69
		0, 3, 0, 0, 0, 0, 0, 0, 4, 0, //79
		0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		};
};


template <class Item> class SimkaTruncateIterator : public TruncateIterator<Item>
{
public:

	SimkaTruncateIterator (Iterator<Item>* ref, u_int64_t limit, bool initRef=true)
        : TruncateIterator<Item>(*ref, limit, initRef), _ref2(0){ setRef(ref); }

private:

    Iterator<Item>* _ref2;
    void setRef (Iterator<Item>* ref2)  { SP_SETATTR(ref2); }

};

template<typename Filter> class SimkaBankFiltered : public BankDelegate
{
public:

    /** Constructor.
     * \param[in] ref : referred bank.
     * \param[in] filter : functor that filters sequence.
     */
	SimkaBankFiltered (IBank* ref, const Filter& filter, const vector<u_int64_t>& nbReadsPerDataset) : BankDelegate (ref), _filter(filter)  {
		_nbReadsPerDataset = nbReadsPerDataset;
	}

    /** \copydoc tools::collections::Iterable::iterator */
    Iterator<Sequence>* iterator ()
    {

    	//cout << "lala" << endl;
        // We create one iterator from the reference
        Iterator<Sequence>* it = _ref->iterator ();

        // We get the composition for this iterator
        std::vector<Iterator<Sequence>*> iterators = it->getComposition();

        if (iterators.size() == 1)  { return new FilterIterator<Sequence,Filter> (it, _filter); }
        else
        {
            // We are going to create a new CompositeIterator, we won't need the one we just got from the reference
            LOCAL(it);

            // We may have to encapsulate each sub iterator with the filter.
            for (size_t i=0; i<iterators.size(); i++)  {

            	//cout << "\t\t" << _nbReadsPerDataset[i] << endl;

            	//Depending on the parameter -max-reads we truncate or not the reads iterator
            	if(_nbReadsPerDataset[i] == 0){

                	//Max nb reads parameter is set to 0. All the reads of each dataset are processed
                	iterators[i] = new FilterIterator<Sequence,Filter> (iterators[i], _filter); ;

            	}
            	else{

                	//We create a truncated iterator that stop processing reads when _nbReadsPerDataset[i] is reached
            		SimkaTruncateIterator<Sequence>* truncIt = new SimkaTruncateIterator<Sequence>(iterators[i], _nbReadsPerDataset[i]);
                	FilterIterator<Sequence,Filter>* filterIt = new FilterIterator<Sequence,Filter> (truncIt, _filter);
                	iterators[i] = filterIt;

            	}

            }
            return new CompositeIterator<Sequence> (iterators);
        }
    }

private:

	vector<u_int64_t> _nbReadsPerDataset;
    Filter _filter;
};



/*********************************************************************
* ** SimkaAlgorithm
*********************************************************************/
template<size_t span=KMER_DEFAULT_SPAN>
class SimkaAlgorithm : public Algorithm
{

public:


    typedef typename Kmer<span>::Type                                       Type;
    typedef typename Kmer<span>::Count                                      Count;
    typedef typename Kmer<span>::ModelCanonical                             ModelCanonical;
    typedef typename ModelCanonical::Kmer                                   KmerType;

	SimkaAlgorithm(IProperties* options);
	~SimkaAlgorithm();
	void execute();
	void print();

	void executeSimkamin();

private:

	void layoutInputFilename();
	void createBank();
	void count();

	void outputMatrix();

	void dumpMatrix(const string& outputFilename, const vector<vector<float> >& matrix);
	void outputHeatmap();
	void __outputHeatmap(const string& outputFilenamePrefix, const string& matrixPercFilename, const string& matrixNormFilename);

	void clear();

	u_int64_t _maxMemory;
	size_t _nbCores;
	string _outputDir;
	string _outputDirTemp;
	size_t _nbBanks;
	string _inputFilename;
	size_t _kmerSize;
	pair<size_t, size_t> _abundanceThreshold;
	SIMKA_SOLID_KIND _solidKind;
	bool _soliditySingle;
	size_t _maxNbReads;
	size_t _minReadSize;
	double _minReadShannonIndex;
	double _minKmerShannonIndex;
	size_t _nbMinimizers;
	//size_t _nbCores;

	SimkaStatistics* _stats;
	SimkaDistance* _simkaDistance;

	string _banksInputFilename;
	vector<string> _tempFilenamesToDelete;
	IBank* _banks;
	IProperties* _options;

	SimkaCountProcessor<span>* _processor;
	vector<string> _bankNames;
	vector<u_int64_t> _nbReadsPerDataset;

	string _outputFilenameSuffix;

	//string _matDksNormFilename;
	//string _matDksPercFilename;
	//string _matAksNormFilename;
	//string _matAksPercFilename;
	//string _heatmapDksFilename;
	//string _heatmapAksFilename;


	size_t _nbPartitions;
    std::vector <std::vector<size_t> > _nbKmersPerPartitionPerBank;
    vector<vector<u_int64_t> > _nbk_per_radix_per_part;//number of kxmer per parti per rad

    /** Temporary partitions management. */
    Storage* _tmpPartitionsStorage;
    void setPartitionsStorage (Storage* tmpPartitionsStorage)  {  SP_SETATTR(tmpPartitionsStorage);  }

    /** Temporary partitions management. */
    Partition<Type>* _tmpPartitions;
    void setPartitions (Partition<Type>* tmpPartitions)  {  SP_SETATTR(tmpPartitions);  }

    vector<u_int64_t> _nbKmerPerPartitions;
    int getSizeofPerItem () const { return Type::getSize()/8 + sizeof(bankIdType); }
    std::vector<size_t> getNbCoresList();
    //this->_local_pInfo.incKmer_and_rad (p, radix_kxmer.getVal(), kx_size); //nb of superkmer per x per parti per radix
};













#endif /* TOOLS_SIMKA_SRC_SIMKAALGORITHM_HPP_ */
