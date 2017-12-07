/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "QCMakeCacheView.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMetaProperty>
#include <QSortFilterProxyModel>
#include <QStyle>

#include "QCMakeWidgets.h"

// filter for searches
class QCMakeSearchFilter : public QSortFilterProxyModel
{
public:
  QCMakeSearchFilter(QObject* o)
    : QSortFilterProxyModel(o)
  {
  }

protected:
  bool filterAcceptsRow(int row, const QModelIndex& p) const CM_OVERRIDE
  {
    QStringList strs;
    const QAbstractItemModel* m = this->sourceModel();
    QModelIndex idx = m->index(row, 0, p);

    // if there are no children, get strings for column 0 and 1
    if (!m->hasChildren(idx)) {
      strs.append(m->data(idx).toString());
      idx = m->index(row, 1, p);
      strs.append(m->data(idx).toString());
    } else {
      // get strings for children entries to compare with
      // instead of comparing with the parent
      int num = m->rowCount(idx);
      for (int i = 0; i < num; i++) {
        QModelIndex tmpidx = m->index(i, 0, idx);
        strs.append(m->data(tmpidx).toString());
        tmpidx = m->index(i, 1, idx);
        strs.append(m->data(tmpidx).toString());
      }
    }

    // check all strings for a match
    foreach (QString const& str, strs) {
      if (str.contains(this->filterRegExp())) {
        return true;
      }
    }

    return false;
  }
};

// filter for searches
class QCMakeAdvancedFilter : public QSortFilterProxyModel
{
public:
  QCMakeAdvancedFilter(QObject* o)
    : QSortFilterProxyModel(o)
    , ShowAdvanced(false)
  {
  }

  void setShowAdvanced(bool f)
  {
    this->ShowAdvanced = f;
    this->invalidate();
  }
  bool showAdvanced() const { return this->ShowAdvanced; }

protected:
  bool ShowAdvanced;

  bool filterAcceptsRow(int row, const QModelIndex& p) const CM_OVERRIDE
  {
    const QAbstractItemModel* m = this->sourceModel();
    QModelIndex idx = m->index(row, 0, p);

    // if there are no children
    if (!m->hasChildren(idx)) {
      bool adv = m->data(idx, QCMakeCacheModel::AdvancedRole).toBool();
      return !adv || this->ShowAdvanced;
    }

    // check children
    int num = m->rowCount(idx);
    for (int i = 0; i < num; i++) {
      bool accept = this->filterAcceptsRow(i, idx);
      if (accept) {
        return true;
      }
    }
    return false;
  }
};

QCMakeCacheView::QCMakeCacheView(QWidget* p)
  : QTreeView(p)
{
  // hook up our model and search/filter proxies
  this->CacheModel = new QCMakeCacheModel(this);
  this->AdvancedFilter = new QCMakeAdvancedFilter(this);
  this->AdvancedFilter->setSourceModel(this->CacheModel);
  this->AdvancedFilter->setDynamicSortFilter(true);
  this->SearchFilter = new QCMakeSearchFilter(this);
  this->SearchFilter->setSourceModel(this->AdvancedFilter);
  this->SearchFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  this->SearchFilter->setDynamicSortFilter(true);
  this->setModel(this->SearchFilter);

  // our delegate for creating our editors
  QCMakeCacheModelDelegate* delegate = new QCMakeCacheModelDelegate(this);
  this->setItemDelegate(delegate);

  this->setUniformRowHeights(true);

  this->setEditTriggers(QAbstractItemView::AllEditTriggers);

  // tab, backtab doesn't step through items
  this->setTabKeyNavigation(false);

  this->setRootIsDecorated(false);
}

bool QCMakeCacheView::event(QEvent* e)
{
  if (e->type() == QEvent::Show) {
    this->header()->setDefaultSectionSize(this->viewport()->width() / 2);
  }
  return QTreeView::event(e);
}

QCMakeCacheModel* QCMakeCacheView::cacheModel() const
{
  return this->CacheModel;
}

QModelIndex QCMakeCacheView::moveCursor(CursorAction act,
                                        Qt::KeyboardModifiers mod)
{
  // want home/end to go to begin/end of rows, not columns
  if (act == MoveHome) {
    return this->model()->index(0, 1);
  }
  if (act == MoveEnd) {
    return this->model()->index(this->model()->rowCount() - 1, 1);
  }
  return QTreeView::moveCursor(act, mod);
}

void QCMakeCacheView::setShowAdvanced(bool s)
{
#if QT_VERSION >= 040300
  // new 4.3 API that needs to be called.  what about an older Qt?
  this->SearchFilter->invalidate();
#endif

  this->AdvancedFilter->setShowAdvanced(s);
}

bool QCMakeCacheView::showAdvanced() const
{
  return this->AdvancedFilter->showAdvanced();
}

void QCMakeCacheView::setSearchFilter(const QString& s)
{
  this->SearchFilter->setFilterFixedString(s);
}

QCMakeCacheModel::QCMakeCacheModel(QObject* p)
  : QStandardItemModel(p)
  , EditEnabled(true)
  , NewPropertyCount(0)
  , View(FlatView)
{
  this->ShowNewProperties = true;
  QStringList labels;
  labels << tr("Name") << tr("Value");
  this->setHorizontalHeaderLabels(labels);
}

QCMakeCacheModel::~QCMakeCacheModel()
{
}

static uint qHash(const QCMakeProperty& p)
{
  return qHash(p.Key);
}

void QCMakeCacheModel::setShowNewProperties(bool f)
{
  this->ShowNewProperties = f;
}

void QCMakeCacheModel::clear()
{
  this->QStandardItemModel::clear();
  this->NewPropertyCount = 0;

  QStringList labels;
  labels << tr("Name") << tr("Value");
  this->setHorizontalHeaderLabels(labels);
}

void QCMakeCacheModel::setProperties(const QCMakePropertyList& props)
{
  QSet<QCMakeProperty> newProps, newProps2;

  if (this->ShowNewProperties) {
    newProps = props.toSet();
    newProps2 = newProps;
    QSet<QCMakeProperty> oldProps = this->properties().toSet();
    oldProps.intersect(newProps);
    newProps.subtract(oldProps);
    newProps2.subtract(newProps);
  } else {
    newProps2 = props.toSet();
  }

  bool b = this->blockSignals(true);

  this->clear();
  this->NewPropertyCount = newProps.size();

  if (View == FlatView) {
    QCMakePropertyList newP = newProps.toList();
    QCMakePropertyList newP2 = newProps2.toList();
    qSort(newP);
    qSort(newP2);
    int row_count = 0;
    foreach (QCMakeProperty const& p, newP) {
      this->insertRow(row_count);
      this->setPropertyData(this->index(row_count, 0), p, true);
      row_count++;
    }
    foreach (QCMakeProperty const& p, newP2) {
      this->insertRow(row_count);
      this->setPropertyData(this->index(row_count, 0), p, false);
      row_count++;
    }
  } else if (this->View == GroupView) {
    QMap<QString, QCMakePropertyList> newPropsTree;
    this->breakProperties(newProps, newPropsTree);
    QMap<QString, QCMakePropertyList> newPropsTree2;
    this->breakProperties(newProps2, newPropsTree2);

    QStandardItem* root = this->invisibleRootItem();

    for (QMap<QString, QCMakePropertyList>::const_iterator iter =
           newPropsTree.begin();
         iter != newPropsTree.end(); ++iter) {
      QString const& key = iter.key();
      QCMakePropertyList const& props2 = iter.value();

      QList<QStandardItem*> parentItems;
      parentItems.append(
        new QStandardItem(key.isEmpty() ? tr("Ungrouped Entries") : key));
      parentItems.append(new QStandardItem());
      parentItems[0]->setData(QBrush(QColor(255, 100, 100)),
                              Qt::BackgroundColorRole);
      parentItems[1]->setData(QBrush(QColor(255, 100, 100)),
                              Qt::BackgroundColorRole);
      parentItems[0]->setData(1, GroupRole);
      parentItems[1]->setData(1, GroupRole);
      root->appendRow(parentItems);

      int num = props2.size();
      for (int i = 0; i < num; i++) {
        QCMakeProperty prop = props2[i];
        QList<QStandardItem*> items;
        items.append(new QStandardItem());
        items.append(new QStandardItem());
        parentItems[0]->appendRow(items);
        this->setPropertyData(this->indexFromItem(items[0]), prop, true);
      }
    }

    for (QMap<QString, QCMakePropertyList>::const_iterator iter =
           newPropsTree2.begin();
         iter != newPropsTree2.end(); ++iter) {
      QString const& key = iter.key();
      QCMakePropertyList const& props2 = iter.value();

      QStandardItem* parentItem =
        new QStandardItem(key.isEmpty() ? tr("Ungrouped Entries") : key);
      root->appendRow(parentItem);
      parentItem->setData(1, GroupRole);

      int num = props2.size();
      for (int i = 0; i < num; i++) {
        QCMakeProperty prop = props2[i];
        QList<QStandardItem*> items;
        items.append(new QStandardItem());
        items.append(new QStandardItem());
        parentItem->appendRow(items);
        this->setPropertyData(this->indexFromItem(items[0]), prop, false);
      }
    }
  }

  this->blockSignals(b);
  this->reset();
}

QCMakeCacheModel::ViewType QCMakeCacheModel::viewType() const
{
  return this->View;
}

void QCMakeCacheModel::setViewType(QCMakeCacheModel::ViewType t)
{
  this->View = t;

  QCMakePropertyList props = this->properties();
  QCMakePropertyList oldProps;
  int numNew = this->NewPropertyCount;
  int numTotal = props.count();
  for (int i = numNew; i < numTotal; i++) {
    oldProps.append(props[i]);
  }

  bool b = this->blockSignals(true);
  this->clear();
  this->setProperties(oldProps);
  this->setProperties(props);
  this->blockSignals(b);
  this->reset();
}

void QCMakeCacheModel::setPropertyData(const QModelIndex& idx1,
                                       const QCMakeProperty& prop, bool isNew)
{
  QModelIndex idx2 = idx1.sibling(idx1.row(), 1);

  this->setData(idx1, prop.Key, Qt::DisplayRole);
  this->setData(idx1, prop.Help, QCMakeCacheModel::HelpRole);
  this->setData(idx1, prop.Type, QCMakeCacheModel::TypeRole);
  this->setData(idx1, prop.Advanced, QCMakeCacheModel::AdvancedRole);

  if (prop.Type == QCMakeProperty::BOOL) {
    int check = prop.Value.toBool() ? Qt::Checked : Qt::Unchecked;
    this->setData(idx2, check, Qt::CheckStateRole);
  } else {
    this->setData(idx2, prop.Value, Qt::DisplayRole);
  }
  this->setData(idx2, prop.Help, QCMakeCacheModel::HelpRole);

  if (!prop.Strings.isEmpty()) {
    this->setData(idx1, prop.Strings, QCMakeCacheModel::StringsRole);
  }

  if (isNew) {
    this->setData(idx1, QBrush(QColor(255, 100, 100)),
                  Qt::BackgroundColorRole);
    this->setData(idx2, QBrush(QColor(255, 100, 100)),
                  Qt::BackgroundColorRole);
  }
}

void QCMakeCacheModel::getPropertyData(const QModelIndex& idx1,
                                       QCMakeProperty& prop) const
{
  QModelIndex idx2 = idx1.sibling(idx1.row(), 1);

  prop.Key = this->data(idx1, Qt::DisplayRole).toString();
  prop.Help = this->data(idx1, HelpRole).toString();
  prop.Type = static_cast<QCMakeProperty::PropertyType>(
    this->data(idx1, TypeRole).toInt());
  prop.Advanced = this->data(idx1, AdvancedRole).toBool();
  prop.Strings =
    this->data(idx1, QCMakeCacheModel::StringsRole).toStringList();
  if (prop.Type == QCMakeProperty::BOOL) {
    int check = this->data(idx2, Qt::CheckStateRole).toInt();
    prop.Value = check == Qt::Checked;
  } else {
    prop.Value = this->data(idx2, Qt::DisplayRole).toString();
  }
}

QString QCMakeCacheModel::prefix(const QString& s)
{
  QString prefix = s.section('_', 0, 0);
  if (prefix == s) {
    prefix = QString();
  }
  return prefix;
}

void QCMakeCacheModel::breakProperties(
  const QSet<QCMakeProperty>& props, QMap<QString, QCMakePropertyList>& result)
{
  QMap<QString, QCMakePropertyList> tmp;
  // return a map of properties grouped by prefixes, and sorted
  foreach (QCMakeProperty const& p, props) {
    QString prefix = QCMakeCacheModel::prefix(p.Key);
    tmp[prefix].append(p);
  }
  // sort it and re-org any properties with only one sub item
  QCMakePropertyList reorgProps;
  QMap<QString, QCMakePropertyList>::iterator iter;
  for (iter = tmp.begin(); iter != tmp.end();) {
    if (iter->count() == 1) {
      reorgProps.append((*iter)[0]);
      iter = tmp.erase(iter);
    } else {
      qSort(*iter);
      ++iter;
    }
  }
  if (reorgProps.count()) {
    tmp[QString()] += reorgProps;
  }
  result = tmp;
}

QCMakePropertyList QCMakeCacheModel::properties() const
{
  QCMakePropertyList props;

  if (!this->rowCount()) {
    return props;
  }

  QVector<QModelIndex> idxs;
  idxs.append(this->index(0, 0));

  // walk the entire model for property entries
  // this works regardless of a flat view or a tree view
  while (!idxs.isEmpty()) {
    QModelIndex idx = idxs.last();
    if (this->hasChildren(idx) && this->rowCount(idx)) {
      idxs.append(this->index(0, 0, idx));
    } else {
      if (!data(idx, GroupRole).toInt()) {
        // get data
        QCMakeProperty prop;
        this->getPropertyData(idx, prop);
        props.append(prop);
      }

      // go to the next in the tree
      while (!idxs.isEmpty() &&
             (
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) &&                                \
  QT_VERSION < QT_VERSION_CHECK(5, 1, 0)
               (idxs.last().row() + 1) >= rowCount(idxs.last().parent()) ||
#endif
               !idxs.last().sibling(idxs.last().row() + 1, 0).isValid())) {
        idxs.remove(idxs.size() - 1);
      }
      if (!idxs.isEmpty()) {
        idxs.last() = idxs.last().sibling(idxs.last().row() + 1, 0);
      }
    }
  }

  return props;
}

bool QCMakeCacheModel::insertProperty(QCMakeProperty::PropertyType t,
                                      const QString& name,
                                      const QString& description,
                                      const QVariant& value, bool advanced)
{
  QCMakeProperty prop;
  prop.Key = name;
  prop.Value = value;
  prop.Help = description;
  prop.Type = t;
  prop.Advanced = advanced;

  // insert at beginning
  this->insertRow(0);
  this->setPropertyData(this->index(0, 0), prop, true);
  this->NewPropertyCount++;
  return true;
}

void QCMakeCacheModel::setEditEnabled(bool e)
{
  this->EditEnabled = e;
}

bool QCMakeCacheModel::editEnabled() const
{
  return this->EditEnabled;
}

int QCMakeCacheModel::newPropertyCount() const
{
  return this->NewPropertyCount;
}

Qt::ItemFlags QCMakeCacheModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags f = QStandardItemModel::flags(idx);
  if (!this->EditEnabled) {
    f &= ~Qt::ItemIsEditable;
    return f;
  }
  if (QCMakeProperty::BOOL == this->data(idx, TypeRole).toInt()) {
    f |= Qt::ItemIsUserCheckable;
  }
  return f;
}

QModelIndex QCMakeCacheModel::buddy(const QModelIndex& idx) const
{
  if (!this->hasChildren(idx) &&
      this->data(idx, TypeRole).toInt() != QCMakeProperty::BOOL) {
    return this->index(idx.row(), 1, idx.parent());
  }
  return idx;
}

QCMakeCacheModelDelegate::QCMakeCacheModelDelegate(QObject* p)
  : QItemDelegate(p)
  , FileDialogFlag(false)
{
}

void QCMakeCacheModelDelegate::setFileDialogFlag(bool f)
{
  this->FileDialogFlag = f;
}

QWidget* QCMakeCacheModelDelegate::createEditor(
  QWidget* p, const QStyleOptionViewItem& /*option*/,
  const QModelIndex& idx) const
{
  QModelIndex var = idx.sibling(idx.row(), 0);
  int type = var.data(QCMakeCacheModel::TypeRole).toInt();
  if (type == QCMakeProperty::BOOL) {
    return CM_NULLPTR;
  }
  if (type == QCMakeProperty::PATH) {
    QCMakePathEditor* editor =
      new QCMakePathEditor(p, var.data(Qt::DisplayRole).toString());
    QObject::connect(editor, SIGNAL(fileDialogExists(bool)), this,
                     SLOT(setFileDialogFlag(bool)));
    return editor;
  }
  if (type == QCMakeProperty::FILEPATH) {
    QCMakeFilePathEditor* editor =
      new QCMakeFilePathEditor(p, var.data(Qt::DisplayRole).toString());
    QObject::connect(editor, SIGNAL(fileDialogExists(bool)), this,
                     SLOT(setFileDialogFlag(bool)));
    return editor;
  }
  if (type == QCMakeProperty::STRING &&
      var.data(QCMakeCacheModel::StringsRole).isValid()) {
    QCMakeComboBox* editor = new QCMakeComboBox(
      p, var.data(QCMakeCacheModel::StringsRole).toStringList());
    editor->setFrame(false);
    return editor;
  }

  QLineEdit* editor = new QLineEdit(p);
  editor->setFrame(false);
  return editor;
}

bool QCMakeCacheModelDelegate::editorEvent(QEvent* e,
                                           QAbstractItemModel* model,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index)
{
  Qt::ItemFlags flags = model->flags(index);
  if (!(flags & Qt::ItemIsUserCheckable) ||
      !(option.state & QStyle::State_Enabled) ||
      !(flags & Qt::ItemIsEnabled)) {
    return false;
  }

  QVariant value = index.data(Qt::CheckStateRole);
  if (!value.isValid()) {
    return false;
  }

  if ((e->type() == QEvent::MouseButtonRelease) ||
      (e->type() == QEvent::MouseButtonDblClick)) {
    // eat the double click events inside the check rect
    if (e->type() == QEvent::MouseButtonDblClick) {
      return true;
    }
  } else if (e->type() == QEvent::KeyPress) {
    if (static_cast<QKeyEvent*>(e)->key() != Qt::Key_Space &&
        static_cast<QKeyEvent*>(e)->key() != Qt::Key_Select) {
      return false;
    }
  } else {
    return false;
  }

  Qt::CheckState state =
    (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked ? Qt::Unchecked
                                                               : Qt::Checked);
  bool success = model->setData(index, state, Qt::CheckStateRole);
  if (success) {
    this->recordChange(model, index);
  }
  return success;
}

// Issue 205903 fixed in Qt 4.5.0.
// Can remove this function and FileDialogFlag when minimum Qt version is 4.5
bool QCMakeCacheModelDelegate::eventFilter(QObject* object, QEvent* evt)
{
  // workaround for what looks like a bug in Qt on Mac OS X
  // where it doesn't create a QWidget wrapper for the native file dialog
  // so the Qt library ends up assuming the focus was lost to something else

  if (evt->type() == QEvent::FocusOut && this->FileDialogFlag) {
    return false;
  }
  return QItemDelegate::eventFilter(object, evt);
}

void QCMakeCacheModelDelegate::setModelData(QWidget* editor,
                                            QAbstractItemModel* model,
                                            const QModelIndex& index) const
{
  QItemDelegate::setModelData(editor, model, index);
  const_cast<QCMakeCacheModelDelegate*>(this)->recordChange(model, index);
}

QSize QCMakeCacheModelDelegate::sizeHint(const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const
{
  QSize sz = QItemDelegate::sizeHint(option, index);
  QStyle* style = QApplication::style();

  // increase to checkbox size
  QStyleOptionButton opt;
  opt.QStyleOption::operator=(option);
  sz = sz.expandedTo(
    style->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt, CM_NULLPTR)
      .size());

  return sz;
}

QSet<QCMakeProperty> QCMakeCacheModelDelegate::changes() const
{
  return mChanges;
}

void QCMakeCacheModelDelegate::clearChanges()
{
  mChanges.clear();
}

void QCMakeCacheModelDelegate::recordChange(QAbstractItemModel* model,
                                            const QModelIndex& index)
{
  QModelIndex idx = index;
  QAbstractItemModel* mymodel = model;
  while (qobject_cast<QAbstractProxyModel*>(mymodel)) {
    idx = static_cast<QAbstractProxyModel*>(mymodel)->mapToSource(idx);
    mymodel = static_cast<QAbstractProxyModel*>(mymodel)->sourceModel();
  }
  QCMakeCacheModel* cache_model = qobject_cast<QCMakeCacheModel*>(mymodel);
  if (cache_model && idx.isValid()) {
    QCMakeProperty prop;
    idx = idx.sibling(idx.row(), 0);
    cache_model->getPropertyData(idx, prop);

    // clean out an old one
    QSet<QCMakeProperty>::iterator iter = mChanges.find(prop);
    if (iter != mChanges.end()) {
      mChanges.erase(iter);
    }
    // now add the new item
    mChanges.insert(prop);
  }
}
