//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "timesigproperties.h"
#include "libmscore/timesig.h"
#include "libmscore/mcursor.h"
#include "libmscore/durationtype.h"
#include "libmscore/score.h"
#include "libmscore/chord.h"
#include "libmscore/measure.h"
#include "libmscore/part.h"
#include "exampleview.h"
#include "menus.h"
#include "musescore.h"
#include "icons.h"

namespace Ms {

//---------------------------------------------------------
//    TimeSigProperties
//---------------------------------------------------------

TimeSigProperties::TimeSigProperties(TimeSig* t, QWidget* parent)
   : QDialog(parent)
      {
      setObjectName("TimeSigProperties");
      setupUi(this);
      fourfourButton->setIcon(*icons[int(Icons::timesig_common_ICON)]);
      allaBreveButton->setIcon(*icons[int(Icons::timesig_allabreve_ICON)]);

      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
      timesig = t;
      pText->setText(timesig->parserString());

      // set validators for numerator and denominator strings
      // which only accept '+', '(', ')', digits and some time symb conventional representations
      // (should be in sync with validator in timedialog.cpp and inspector.cpp)
      QRegExp rx("[0-9+CO()|=,x ~X\\*\\-\\[\\]\\.\\/\\x00A2\\x00D8\\x00BC\\x00BD\\x00BE\\xE097\\xE098\\xE099\\xE09A\\xE09B\\xE09C\\xE09D]*");
      QValidator *validator = new QRegExpValidator(rx, this);
      pText->setValidator(validator);

      Fraction nominal = timesig->sig() / timesig->stretch();
      nominal.reduce();
      zNominal->setValue(nominal.numerator());
      nNominal->setValue(nominal.denominator());
      Fraction sig(timesig->sig());
      zActual->setValue(sig.numerator());
      nActual->setValue(sig.denominator());
      zNominal->setEnabled(false);
      nNominal->setEnabled(false);

      connect(pText,     SIGNAL(textChanged(const QString&)),    SLOT(textChanged()));

       // TODO: fix http://musescore.org/en/node/42341
      // for now, editing of actual (local) time sig is disabled in dialog
      // but more importantly, the dialog should make it clear that this is "local" change only
      // and not normally the right way to add 7/4 to a score
      zActual->setEnabled(false);
      nActual->setEnabled(false);
      switch (timesig->timeSigType()) {
            case TimeSigType::NORMAL:
                  textButton->setChecked(true);
                  break;
            case TimeSigType::FOUR_FOUR:
                  fourfourButton->setChecked(true);
                  break;
            case TimeSigType::ALLA_BREVE:
                  allaBreveButton->setChecked(true);
                  break;
            case TimeSigType::CUT_BACH:
//                  cutBachButton->setChecked(true);
//                  break;
            case TimeSigType::CUT_TRIPLE:
//                  cutTripleButton->setChecked(true);
                  break;
            }

      // set ID's of other symbols
      struct ProlatioTable {
            SymId id;
            Icons icon;
            };
      static const std::vector<ProlatioTable> prolatioList = {
            { SymId::mensuralProlation1,  Icons::timesig_prolatio01_ICON },  // tempus perfectum, prol. perfecta
            { SymId::mensuralProlation2,  Icons::timesig_prolatio02_ICON },  // tempus perfectum, prol. imperfecta
            { SymId::mensuralProlation3,  Icons::timesig_prolatio03_ICON },  // tempus perfectum, prol. imperfecta, dimin.
            { SymId::mensuralProlation4,  Icons::timesig_prolatio04_ICON },  // tempus perfectum, prol. perfecta, dimin.
            { SymId::mensuralProlation5,  Icons::timesig_prolatio05_ICON },  // tempus imperf. prol. perfecta
            { SymId::mensuralProlation7,  Icons::timesig_prolatio07_ICON },  // tempus imperf., prol. imperfecta, reversed
            { SymId::mensuralProlation8,  Icons::timesig_prolatio08_ICON },  // tempus imperf., prol. perfecta, dimin.
            { SymId::mensuralProlation10, Icons::timesig_prolatio10_ICON },  // tempus imperf., prol imperfecta, dimin., reversed
            { SymId::mensuralProlation11, Icons::timesig_prolatio11_ICON },  // tempus inperf., prol. perfecta, reversed
            };

      ScoreFont* scoreFont = gscore->scoreFont();
      int idx = 0;
      otherCombo->clear();
      for (ProlatioTable pt : prolatioList) {
            const QString& str = scoreFont->toString(pt.id);
            if (str.size() > 0) {
                  otherCombo->addItem(*icons[int(pt.icon)],"", int(pt.id));
                  // if time sig matches this symbol string, set as selected
                  if (timesig->timeSigType() == TimeSigType::NORMAL && timesig->parserString() == str) {
                        textButton->setChecked(false);
                        otherButton->setChecked(true);
                        otherCombo->setCurrentIndex(idx);
                        // set the custom text field to empty
                        pText->setText(QString());
                        }
                  }
            idx++;
            }

      Groups g = t->groups();
      if (g.empty())
            g = Groups::endings(timesig->sig());     // initialize with default
      groups->setSig(timesig->sig(), g, timesig->parserString());

      MuseScore::restoreGeometry(this);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void TimeSigProperties::accept()
      {
      TimeSigType ts = TimeSigType::NORMAL;
      if (textButton->isChecked() || otherButton->isChecked())
            ts = TimeSigType::NORMAL;
      else if (fourfourButton->isChecked())
            ts = TimeSigType::FOUR_FOUR;
      else if (allaBreveButton->isChecked())
            ts = TimeSigType::ALLA_BREVE;

      Fraction actual(zActual->value(), nActual->value());
      Fraction nominal(zNominal->value(), nNominal->value());
      timesig->setSig(actual, ts);
      timesig->setStretch(nominal / actual);

      if (pText->text() != timesig->parserString())
            timesig->setParserString(pText->text());

      if (otherButton->isChecked()) {
            ScoreFont* scoreFont = timesig->score()->scoreFont();
            SymId symId = (SymId)( otherCombo->itemData(otherCombo->currentIndex()).toInt() );
            // set timesig string to font string for symbol
            timesig->setParserString(scoreFont->toString(symId));
            }

      Groups g = groups->groups();
      timesig->setGroups(g);
      QDialog::accept();
      }

//---------------------------------------------------------
//   hideEvent
//---------------------------------------------------------

void TimeSigProperties::hideEvent(QHideEvent* event)
      {
      MuseScore::saveGeometry(this);
      QWidget::hideEvent(event);
      }

//---------------------------------------------------------
//   textChanged
//---------------------------------------------------------

void TimeSigProperties::textChanged()
      {
      // if text is changed, delete (old version) num/den strings
      Fraction sig(timesig->sig());
      groups->setSig(sig, Groups::endings(sig), pText->text());
      }
}

