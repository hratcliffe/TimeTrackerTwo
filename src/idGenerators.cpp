//
//  idGenerators.cpp
//  
//
//  Created by Heather Ratcliffe on 15/06/2018.
//
//

#include "idGenerators.h"

seqIdGenerator::seqIdGenerator(IdGenerator * src){
/** \brief Copy constructor for sequential ID generator
*
*
  @param src Generator to copy
  \todo Do not use name for matching
*/

  setup();
  if(src->name == this->name) this->counter = dynamic_cast<const seqIdGenerator *>(src)->counter;
}

void seqIdGenerator::setup(){
/** \brief Setup function for sequential ID generator
*
*
*/

  this->name = "Seq";
  this->counter = 1000;
}

proIds::Uuid seqIdGenerator::getNextId(){
/** \brief Get next sequential ID
@returns Next sequential proIds::Uuid
*/

  int temp = counter;
  counter ++;
  return proIds::Uuid(std::to_string(temp));
}

proIds::Uuid seqIdGenerator::getNullId(){
/** \brief Get null sequential ID
@returns Null sequential proIds::Uuid
*/

  return proIds::NullUid;
}


proIds::Uuid seqIdGenerator::getNextId(proIds::uidTag tag){
/** \brief Get next sequential ID with tag
*
* Gets a new sequential ID, with specified tag
  @param tag Tag to use
  @returns Next sequential proIds::Uuid
*/

  proIds::Uuid temp = getNextId();
  temp.tag(tag);
  return temp;
}
