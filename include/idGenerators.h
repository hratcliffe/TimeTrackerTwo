//
//  idGenerators.h
//  
//
//  Created by Heather Ratcliffe on 15/06/2018.
//
//

#ifndef _idGenerators_h
#define _idGenerators_h

#include <ctime>
#include <string>
#include <vector>
#include <iostream>

#include <QUuid>

namespace proIds{

/** \brief Namespace wrapping all ID code
*
* Allows us to swap out the QT Uid stuff for
 plain string based IDs for e.g. sequential stuff 
 */

  /** \brief Type of ID
  *
  * Type of IDs in use, e.g. string, QUuid etc
  **/
  enum class uidType {str, qt};

  /** \brief Tag for id content type
  *
  * Tag defining what sort of thing is under this ID 
  **/
  enum class uidTag {none, sub, oneoff};

  /** \brief Wrapper class for Uid
  *
  * Wraps some sort of Unique ID, so can contain, create etc any of the valid sorts.
  \todo Make map insertable if not
  **/
  class uidWrapper{
    private:
      QUuid qID;/**< \brief QT supplied uid */
      std::string sID;/**< \brief Any sort of string holding uid*/
      uidTag Itag;/**< \brief Tag for content type*/
      uidType useType = uidType::str;/**< \brief Default type to use*/
  
    public:
      /** \brief Default constructor
      *
      *
       Construct Null Uuid object
      */
      uidWrapper(){this->qID = QUuid(); this->sID = "0000"; this->Itag = uidTag::none;}
      /** \brief Constructor from string
      *
      *
       Construct sequential ID from string
      @param sID Input string
      \todo Should roll counter forwards to keep sequence
       */
      uidWrapper(std::string sID){this->sID = sID;}
      /** \brief Constructor from string
      *
      *
       Construct tagged sequential ID from string
       @param sID input string
       @param tag Tag to apply
       \todo Should use untagged version and add tag
       */
      uidWrapper(std::string sID, uidTag tag){this->sID = sID; this->Itag = tag;}

      /** \brief Constructor from QUid
      *
      *
       Construct uid from QUid
       */
      uidWrapper(QUuid qID){this->qID = qID;}

      /** \brief Apply tag
        @param tag Tag to apply
      */
      void tag(uidTag tag){this->Itag = tag;}

      /** \brief Check for equality
      *
      * Equality checks only core id equality. C.f. isExactEq
        @param other Object to compare
      */
      bool isEq(const uidWrapper &other)const{return sID == other.sID;};
      /** \brief Check for exact equality
      *
      * Exact equality includes tag C.f. isEq
        @param other Object to compare
      */
      bool isExactEq(const uidWrapper &other)const{return sID == other.sID && Itag == other.Itag;};
      friend std::ostream& operator<<(std::ostream& stream, const uidWrapper& uid);

      /** \brief Stringify
        \todo Wont work for non string ids
      */
      std::string to_string()const{return sID;}
  };
  
  /** \brief Check equality of uids
  *
    @param lhs First object
    @param rhs Other object
    @returns Boolean true if equals, false else
  */
  inline bool operator==(const uidWrapper &lhs, const uidWrapper & rhs){ return lhs.isEq(rhs);};
  /** \brief Check nonequality of uids
  *
    @param lhs First object
    @param rhs Other object
    @returns Boolean true if not-equal, false else
  */
  inline bool operator!=(const uidWrapper &lhs, const uidWrapper & rhs){ return !(lhs==rhs);};

  /** \brief Stream operator for id
  \todo Use tostring operator
  */
  inline std::ostream& operator<< (std::ostream& stream, const uidWrapper& uid){ stream<<uid.sID; return stream;};

  /** \brief Unique Id type
  *
  * Shortened name for unique id type
  */
  typedef uidWrapper Uuid;

  /** \brief Null unique ID
  *
  * Convenience name for null object
  */
  const static Uuid NullUid = uidWrapper();

};

/** \brief Id generator parent class
*
* ID generators should generate unique ids (any method fine), should have a unique name for each generator. They should implement getNextId() to get a new Uid and getNullId() to get a null Uid, as well as getNextId(proIds::uidTag) to get a new tagged Uid. 
* If generator has internal state, it should implement sufficient copy constructors to carry this over if a copy is made.
*/
class IdGenerator{

  protected:

  public:
    std::string name = "";/**< \brief Name of generator */

    /** \brief Default constructor */
    IdGenerator(){name = "Basic";};

    /** \brief Destructor */
    virtual ~IdGenerator(){;};

    /** \brief Get null unique id*/
    virtual proIds::Uuid getNextId() = 0;
    /** \brief Get null id*/
    virtual proIds::Uuid getNullId(){return proIds::NullUid;};

    /** \brief Get tagged next id*/
    virtual proIds::Uuid getNextId(proIds::uidTag tag) = 0;
};

/** \brief Sequential id generator
*
* Generates IDs of simple sequential type. These are fine where not many are needed, but require careful handling if reading in old data.
*/
class seqIdGenerator : public IdGenerator{

  protected:
    int counter = -1;/**< \brief Current id number */

  public:

    /** \brief Default constructor */
    seqIdGenerator(){setup();};

    /** \brief Destructor */
    virtual ~seqIdGenerator(){;};

    /** \brief Copy constructor*/
    seqIdGenerator(const seqIdGenerator & src){this->name = src.name; this->counter = src.counter;};
    seqIdGenerator(IdGenerator * src);
    void setup();

    virtual proIds::Uuid getNextId();
    virtual proIds::Uuid getNullId();
    virtual proIds::Uuid getNextId(proIds::uidTag tag);

};

/** \brief Unique id generator
*
* Uses QTs Uuid generator to create true unique ids. These take more memory but are unique enough not to worry about any previous invocations
*/

class uniqueIdGenerator : public IdGenerator{

  private:

  public:
    std::string name = "Uniq";/**< \brief Name of generator */

    /** \brief Destructor */
    virtual ~uniqueIdGenerator(){;};
    /** \brief Not yet implemented*/
    virtual proIds::Uuid getNextId(){return proIds::NullUid;};
    /** \brief Get null unique id*/
    virtual proIds::Uuid getNullId(){return proIds::NullUid;};
    /** \brief Not yet implemented*/
    virtual proIds::Uuid getNextId(proIds::uidTag tag){return proIds::NullUid;};
};


#endif
