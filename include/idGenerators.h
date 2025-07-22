//
//  idGenerators.h
//  
//
//  Created by Heather Ratcliffe on 15/06/2018.
// Modified 2025
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

  /** \brief Tag for id content type
  *
  * Tag defining what sort of thing is under this ID 
  **/
  enum class uidTag {none, sub, oneoff};

  /** \brief Wrapper class for Uid
  *
  **/
  class uidWrapper{
    private:
      QUuid qID;/**< \brief QT supplied uid */
      uidTag Itag=uidTag::none;/**< \brief Tag for content type*/
  
    public:
      /** \brief Generic - a uid with none tag */
      uidWrapper(){this->qID = QUuid(); this->Itag = uidTag::none;}

      /** \brief Constructor from QUid */
      uidWrapper(QUuid qID){this->qID = qID;}

      /** \brief Constructor from QUid with tag */
      uidWrapper(QUuid qID, uidTag tag){this->qID = qID; this->Itag = tag;}

      uidWrapper(std::string str){
        /** \brief Constructor from string
        *
        * Converts a string to a QUuid and sets the tag to none
        */
        this->qID = QUuid::fromString(QString::fromStdString(str));
        this->Itag = uidTag::none;
      }
 
      /** \brief Apply tag
        @param tag Tag to apply
      */
      void tag(uidTag tag){this->Itag = tag;}

      bool isTaggedAs(uidTag tag)const{
        /** \brief Check if tagged as
        *
        * @param tag Tag to check against
        * @returns Boolean true if tagged as, false else
        */
        return Itag == tag;
      }

      /** \brief Check for equality
      *
      * Equality checks only core id equality. C.f. isExactEq
        @param other Object to compare
      */
      bool isEq(const uidWrapper &other)const{return qID == other.qID;};
      /** \brief Check for exact equality
      *
      * Exact equality includes tag C.f. isEq
        @param other Object to compare
      */
      bool isExactEq(const uidWrapper &other)const{return qID == other.qID && Itag == other.Itag;};
      friend std::ostream& operator<<(std::ostream& stream, const uidWrapper& uid);

      bool operator<(const uidWrapper &other)const{
        /** \brief Less than operator
        *
        * Compares only the core id, not the tag
          @param other Object to compare
          @returns Boolean true if less than, false else
        */
        return qID < other.qID;
      }

      /** \brief Stringify
      */
      std::string to_string()const{return qID.toString().toStdString();}
  };
  
  inline bool operator==(const uidWrapper &lhs, const uidWrapper & rhs){ return lhs.isEq(rhs);};
  inline bool operator!=(const uidWrapper &lhs, const uidWrapper & rhs){ return !(lhs==rhs);};

  inline std::ostream& operator<< (std::ostream& stream, const uidWrapper& uid){ stream<<uid.to_string(); return stream;};

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
    /** \brief Next Id*/
    virtual proIds::Uuid getNextId(){return QUuid::createUuid();};
    /** \brief Get null unique id*/
    virtual proIds::Uuid getNullId(){return proIds::NullUid;};
    /** \brief Not yet implemented*/
    virtual proIds::Uuid getNextId(proIds::uidTag tag){return proIds::Uuid(QUuid::createUuid(), tag);};
};


#endif
