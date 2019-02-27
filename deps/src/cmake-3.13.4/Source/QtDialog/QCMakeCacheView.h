/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef QCMakeCacheView_h
#define QCMakeCacheView_h

#include "QCMake.h"

#include <QItemDelegate>
#include <QSet>
#include <QStandardItemModel>
#include <QTreeView>

class QSortFilterProxyModel;
class QCMakeCacheModel;
class QCMakeAdvancedFilter;

/// Qt view class for cache properties
class QCMakeCacheView : public QTreeView
{
  Q_OBJECT
public:
  QCMakeCacheView(QWidget* p);

  // retrieve the QCMakeCacheModel storing all the pointers
  // this isn't necessarily the model one would get from model()
  QCMakeCacheModel* cacheModel() const;

  // get whether to show advanced entries
  bool showAdvanced() const;

  QSize sizeHint() const { return QSize(200, 200); }

public slots:
  // set whether to show advanced entries
  void setShowAdvanced(bool);
  // set the search filter string.  any property key or value not matching will
  // be filtered out
  void setSearchFilter(const QString&);

protected:
  QModelIndex moveCursor(CursorAction, Qt::KeyboardModifiers);
  bool event(QEvent* e);
  QCMakeCacheModel* CacheModel;
  QCMakeAdvancedFilter* AdvancedFilter;
  QSortFilterProxyModel* SearchFilter;
};

/// Qt model class for cache properties
class QCMakeCacheModel : public QStandardItemModel
{
  Q_OBJECT
public:
  QCMakeCacheModel(QObject* parent);
  ~QCMakeCacheModel();

  // roles used to retrieve extra data such has help strings, types of
  // properties, and the advanced flag
  enum
  {
    HelpRole = Qt::ToolTipRole,
    TypeRole = Qt::UserRole,
    AdvancedRole,
    StringsRole,
    GroupRole
  };

  enum ViewType
  {
    FlatView,
    GroupView
  };

public slots:
  // set a list of properties.  This list will be sorted and grouped according
  // to prefix.  Any property that existed already and which is found in this
  // list of properties to set will become an old property.  All others will
  // become new properties and be marked red.
  void setProperties(const QCMakePropertyList& props);

  // set whether to show new properties in red
  void setShowNewProperties(bool);

  // clear everything from the model
  void clear();

  // set flag whether the model can currently be edited.
  void setEditEnabled(bool);

  // insert a new property at a row specifying all the information about the
  // property
  bool insertProperty(QCMakeProperty::PropertyType t, const QString& name,
                      const QString& description, const QVariant& value,
                      bool advanced);

  // set the view type
  void setViewType(ViewType t);
  ViewType viewType() const;

public:
  // get the properties
  QCMakePropertyList properties() const;

  // editing enabled
  bool editEnabled() const;

  // returns how many new properties there are
  int newPropertyCount() const;

  // return flags (overloaded to modify flag based on EditEnabled flag)
  Qt::ItemFlags flags(const QModelIndex& index) const;
  QModelIndex buddy(const QModelIndex& idx) const;

  // get the data in the model for this property
  void getPropertyData(const QModelIndex& idx1, QCMakeProperty& prop) const;

protected:
  bool EditEnabled;
  int NewPropertyCount;
  bool ShowNewProperties;
  ViewType View;

  // set the data in the model for this property
  void setPropertyData(const QModelIndex& idx1, const QCMakeProperty& p,
                       bool isNew);

  // breaks up he property list into groups
  // where each group has the same prefix up to the first underscore
  static void breakProperties(const QSet<QCMakeProperty>& props,
                              QMap<QString, QCMakePropertyList>& result);

  // gets the prefix of a string up to the first _
  static QString prefix(const QString& s);
};

/// Qt delegate class for interaction (or other customization)
/// with cache properties
class QCMakeCacheModelDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  QCMakeCacheModelDelegate(QObject* p);
  /// create our own editors for cache properties
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const;
  bool editorEvent(QEvent* event, QAbstractItemModel* model,
                   const QStyleOptionViewItem& option,
                   const QModelIndex& index);
  bool eventFilter(QObject* object, QEvent* event);
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const;

  QSet<QCMakeProperty> changes() const;
  void clearChanges();

protected slots:
  void setFileDialogFlag(bool);

protected:
  bool FileDialogFlag;
  // record a change to an item in the model.
  // this simply saves the item in the set of changes
  void recordChange(QAbstractItemModel* model, const QModelIndex& index);

  // properties changed by user via this delegate
  QSet<QCMakeProperty> mChanges;
};

#endif
