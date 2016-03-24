/// array_set.hpp

#pragma once

#include <algorithm>
#include "std/vector.hpp"
#include <utility>
#include <functional>

#pragma pack(push,8)
#pragma warning(push,3)

namespace mew
{
	template<class T, bool Unique= false,class Pred = std::less<T>, class Alloc = std::allocator<T> >
	class array_set
	{
	public:
		typedef array_set<T,Unique,Pred,Alloc> self_type;
		typedef std::vector<T,Alloc>		Cont;
		typedef typename Cont::allocator_type	allocator_type;
		typedef typename Cont::size_type		size_type;
		typedef typename Cont::difference_type	difference_type;
		typedef typename Cont::reference		reference;
		typedef typename Cont::const_reference	const_reference;
		typedef typename Cont::value_type		value_type;
		typedef T								key_type;
		typedef typename Cont::iterator			iterator;
		typedef typename Cont::const_iterator	const_iterator;
		typedef Pred								key_compare;
		typedef Pred								value_compare;

		typedef typename Cont::const_reverse_iterator	const_reverse_iterator;
		typedef typename Cont::reverse_iterator			reverse_iterator;

		typedef std::pair<iterator, iterator>				Pairii_;
		typedef std::pair<const_iterator, const_iterator>	Paircc_;
		typedef std::pair<iterator, bool>					Pairib_;

	public:
		explicit array_set(const Pred& pred = Pred(),const Alloc& al = Alloc())
			:key_compare_(pred),vec_(al){}
	template<class It>
	array_set(It first, It beyond, const Pred& pred = Pred(),const Alloc& al = Alloc())
		:key_compare_(pred),vec_(first,beyond,al)
        {stable_sort();}
	array_set(const self_type& x)
		: vec_(x.vec_),key_compare_(x.key_compare_)
        {}
    ~array_set()                {}
    self_type& operator=(const self_type& x) {vec_.operator=(x.vec_);
                                     key_compare_= x.key_compare_;
                                     return *this;}
    self_type& operator=(const Cont& x){vec_.operator=(x);
                                    sort();return *this;}
		
	void				reserve(size_type n)	{vec_.reserve(n);}
	iterator			begin()					{return vec_.begin(); }
	const_iterator		begin() const			{return vec_.begin(); }
    iterator			end()					{return vec_.end();}
    const_iterator		end() const				{return vec_.end();}
    reverse_iterator	rbegin()				{return vec_.rbegin();}
    const_reverse_iterator rbegin() const		{return vec_.rbegin();}
    reverse_iterator rend()						{return vec_.rend();}
    const_reverse_iterator rend() const			{return vec_.rend();}

    size_type size() const						{return vec_.size();}
    size_type max_size() const					{return vec_.max_size();}
    bool empty() const							{return vec_.empty();}
    Alloc get_allocator() const						{return vec_.get_allocator();}
    const_reference at(size_type p) const		{return vec_.at(p);}
    reference at(size_type p)					{return vec_.at(p);}
	const_reference operator[](size_type p) const	{return vec_.operator[](p);}
		
	reference operator[](size_type p)			{return vec_.operator[](p);}
    reference front()							{return vec_.front();}
	const_reference front() const				{return vec_.front();}
    reference back()							{return vec_.back();}
    const_reference back() const				{return vec_.back();}
    void pop_back()								{vec_.pop_back();}

    void assign(const_iterator first, const_iterator beyond)	{vec_.assign(first,beyond);}
	void assign(size_type n, const T& x = T())					{vec_.assign(n,x);}

/*insert members*/
   Pairib_ insert(const value_type& x)
		{
            if(Unique){
                iterator p= lower_bound(x);
                if(p==end()||key_compare_(x,*p)){
                    return Pairib_(InsertImpl_(p,x),true);
                }else{
                    return Pairib_(p,false);
                }
            }else{
                iterator p= upper_bound(x);
                return Pairib_(InsertImpl_(p,x),true);
            }
        }
   iterator insert(iterator it, const value_type& hint)
        {
           if(it!=end() ){
               if(Unique){
                   if(key_compare_(*it,hint)){
                       if((it+1)==end()||KeyCompare_Gt_(*(it+1),hint)){//use hint
                            return InsertImpl_(it+1,hint);
                       }else if(KeyCompare_Geq_(*(it+1),hint)){
                           return end();
                       }
                    }
               }else{
                   if(	KeyCompare_Leq_(*it,hint)
					   &&((it+1)==end()||KeyCompare_Geq_(*(it+1),hint))){
                       return InsertImpl_(it+1,hint);
                   }
               }
           }
           return insert(hint).first;
        }
	template<class It>
	void insert(It first, It beyond)
    {
        size_type n= std::distance(first,beyond);
        reserve(size()+n);
        for( ;first!=beyond;++first){
            insert(*first);
        }
    }
    iterator erase(iterator p)          {return vec_.erase(p);}
	iterator erase(iterator first, iterator beyond)
                                        {return vec_.erase(first,beyond);}
	template < class U > size_type erase(const U& key)
    {
        Pairii_ begEnd= equal_range(key);
        size_type n= std::distance(begEnd.first,begEnd.second);
        erase(begEnd.first,begEnd.second);
        return n;
    }
    void clear()                        {return vec_.clear();}
		
    bool Eq_(const self_type& x) const      
		{return (size() == x.size()
		&& std::equal(begin(), end(), x.begin())); }
	bool Lt_(const self_type& x) const
        {return (std::lexicographical_compare(begin(), end(),
										x.begin(), x.end()));}
	void swap(self_type& x)
        {vec_.swap(x.vec_);std::swap(key_compare_,x.key_compare_);}
        
	friend void swap(self_type& x, self_type& Y_)
		{x.swap(Y_); }

	key_compare&   key_comp()			{return key_compare_; }
    value_compare& value_comp()		{return (key_comp()); }
	const key_compare&   key_comp() const			{return key_compare_; }
    const value_compare& value_comp() const		{return (key_comp()); }

	template < class U >
	iterator find(const U& k)
		{	iterator p = lower_bound(k);
			return (p==end()||key_compare_(k, *p))? end():p;
		}
	template < class U >
	const_iterator find(const U& k) const
		{const_iterator p = lower_bound(k);
        return (p==end()||key_compare_(k,*p))?end():p;}
	size_type count(const T& k) const
		{Paircc_ Ans_ = equal_range(k);
        size_type n = std::distance(Ans_.first, Ans_.second);
        return (n); }
	template < class U >       iterator lower_bound(const U& k)			{ return std::lower_bound(begin(), end(), k, key_compare_); }
	template < class U > const_iterator lower_bound(const U& k) const	{ return std::lower_bound(begin(), end(), k, key_compare_); }
	template < class U >       iterator upper_bound(const U& k)			{return std::upper_bound(begin(), end(), k, key_compare_); }
	template < class U > const_iterator upper_bound(const U& k) const	{return std::upper_bound(begin(), end(), k, key_compare_); }
	template < class U > Pairii_ equal_range(const U& k)				{return std::equal_range(begin(), end(), k, key_compare_); }
	template < class U > Paircc_ equal_range(const U& k) const			{return std::equal_range(begin(), end(), k, key_compare_); }

/*functions for use with direct std::vector-access*/
    Cont& get_container()
        {return vec_;}
    void sort()//restore sorted order after low level access 
        {   std::sort(vec_.begin(),vec_.end(),key_compare_);
            if( Unique ){
                vec_.erase(Unique_(),vec_.end());
            }
        }
    void stable_sort()//restore sorted order after low level access 
        {   std::stable_sort(vec_.begin(),vec_.end(),key_compare_);
            if( Unique ){
                erase(Unique_(),end());
            }
        }   
protected:
    iterator Unique_()
        {   iterator front_= vec_.begin(),out_= vec_.end(),end_=vec_.end();
            bool bCopy_= false;
            for(iterator prev_; (prev_=front_)!=end_ && ++front_!=end_; ){
                if( key_compare_(*prev_,*front_)){
                    if(bCopy_){
                        *out_= *front_;
                        out_++;
                    }
                }else{
                    if(!bCopy_){out_=front_;bCopy_=true;}
                }
            }
            return out_;
        }
    iterator InsertImpl_(iterator p,const value_type& x)
        {return vec_.insert(p,x);}
    bool KeyCompare_Leq_(const T& ty0,const T& ty1)
        {return !key_compare_(ty1,ty0);}
    bool KeyCompare_Geq_(const T& ty0,const T& ty1)
        {return !key_compare_(ty0,ty1);}
    bool KeyCompare_Gt_(const T& ty0,const T& ty1)
        {return key_compare_(ty1,ty0);}

    key_compare         key_compare_;
    Cont                vec_;
};


template<class T,bool Unique,class Pred, class Alloc> inline
	bool operator==(const array_set<T, Unique,Pred,Alloc>& x,
		            const array_set<T, Unique,Pred,Alloc>& Y_)
	{return x.Eq_(Y_); }
template<class T,bool Unique,class Pred, class Alloc> inline
	bool operator!=(const array_set<T, Unique,Pred,Alloc>& x,
		            const array_set<T, Unique,Pred,Alloc>& Y_)
	{return !(x == Y_); }
template<class T,bool Unique,class Pred, class Alloc> inline
	bool operator<(const array_set<T, Unique,Pred,Alloc>& x,
		            const array_set<T, Unique,Pred,Alloc>& Y_)
	{return x.Lt_(Y_);}
template<class T,bool Unique,class Pred,class Alloc> inline
	bool operator>(const array_set<T, Unique,Pred,Alloc>& x,
		            const array_set<T, Unique,Pred,Alloc>& Y_)
	{return Y_ < x; }
template<class T,bool Unique,class Pred, class Alloc> inline
	bool operator<=(const array_set<T, Unique,Pred,Alloc>& x,
		            const array_set<T, Unique,Pred,Alloc>& Y_)
	{return !(Y_ < x); }
template<class T, bool Unique,class Pred,class Alloc> inline
	bool operator>=(const array_set<T, Unique,Pred,Alloc>& x,
		            const array_set<T, Unique,Pred,Alloc>& Y_)
	{return (!(x < Y_)); }
}

#pragma warning(pop)
#pragma pack(pop)

