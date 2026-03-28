//
//  qExtensionObjects.h
//  
//
//  Created by Heather Ratcliffe on 17/06/2018.
//
//

#ifndef _projectButton_h
#define _projectButton_h

#include <QPushButton>

#include "idGenerators.h"

/** \brief Button with project details
*
* Extends QPushButton to add project name, UID and special flag.
*/
class projectButton : public QPushButton{

  private:
    bool is_special;/**< \brief Flag for project button or special button (close etc) */

  public:

    projectButton(QWidget * parent = nullptr) : QPushButton(parent), is_special(false){};
    std::string fullName;/**< \brief Displayed project name */
    proIds::Uuid projectId;/**< \brief Uid to identify project */

    /** \brief Set special flat to true */
    void mark_special(){this->is_special = true;};
  
};


#endif
