/****************************************************************************
** Meta object code from reading C++ file 'monitormanager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/monitormanager.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'monitormanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14MonitorManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto MonitorManager::qt_create_metaobjectdata<qt_meta_tag_ZN14MonitorManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MonitorManager",
        "statusMessageChanged",
        "",
        "busyChanged",
        "operationFinished",
        "success",
        "profile",
        "QVariantMap",
        "row",
        "saveProfile",
        "values",
        "removeProfile",
        "startProfile",
        "stopProfile",
        "refresh",
        "generatePassword",
        "statusMessage",
        "busy",
        "backendAvailable",
        "backendDescription"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'statusMessageChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'busyChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'operationFinished'
        QtMocHelpers::SignalData<void(bool)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 5 },
        }}),
        // Method 'profile'
        QtMocHelpers::MethodData<QVariantMap(int) const>(6, 2, QMC::AccessPublic, 0x80000000 | 7, {{
            { QMetaType::Int, 8 },
        }}),
        // Method 'saveProfile'
        QtMocHelpers::MethodData<void(const QVariantMap &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 10 },
        }}),
        // Method 'removeProfile'
        QtMocHelpers::MethodData<void(int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Method 'startProfile'
        QtMocHelpers::MethodData<void(int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Method 'stopProfile'
        QtMocHelpers::MethodData<void(int)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Method 'refresh'
        QtMocHelpers::MethodData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'generatePassword'
        QtMocHelpers::MethodData<QString() const>(15, 2, QMC::AccessPublic, QMetaType::QString),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'statusMessage'
        QtMocHelpers::PropertyData<QString>(16, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'busy'
        QtMocHelpers::PropertyData<bool>(17, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'backendAvailable'
        QtMocHelpers::PropertyData<bool>(18, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'backendDescription'
        QtMocHelpers::PropertyData<QString>(19, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MonitorManager, qt_meta_tag_ZN14MonitorManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MonitorManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MonitorManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MonitorManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14MonitorManagerE_t>.metaTypes,
    nullptr
} };

void MonitorManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MonitorManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->statusMessageChanged(); break;
        case 1: _t->busyChanged(); break;
        case 2: _t->operationFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: { QVariantMap _r = _t->profile((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->saveProfile((*reinterpret_cast<std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 5: _t->removeProfile((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->startProfile((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->stopProfile((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->refresh(); break;
        case 9: { QString _r = _t->generatePassword();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MonitorManager::*)()>(_a, &MonitorManager::statusMessageChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorManager::*)()>(_a, &MonitorManager::busyChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorManager::*)(bool )>(_a, &MonitorManager::operationFinished, 2))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->statusMessage(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->busy(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->backendAvailable(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->backendDescription(); break;
        default: break;
        }
    }
}

const QMetaObject *MonitorManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MonitorManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MonitorManagerE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int MonitorManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void MonitorManager::statusMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MonitorManager::busyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MonitorManager::operationFinished(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP
