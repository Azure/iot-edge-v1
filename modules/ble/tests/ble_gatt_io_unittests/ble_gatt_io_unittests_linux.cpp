// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include <glib.h>
#include <gio/gio.h>

#include "gio_async_seq.h"
#include "ble_gatt_io.h"
#include "bluez_device.h"
#include "bluez_characteristic.h"
#include "azure_c_shared_utility/lock.h"

DEFINE_MICROMOCK_ENUM_TO_STRING(BLEIO_GATT_RESULT, BLEIO_GATT_RESULT_VALUES);

static MICROMOCK_MUTEX_HANDLE g_testByTest;
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
    /*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/
#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit

#include "../../src/gio_async_seq.c"
};

#define FILL_UUID(p) \
    for (size_t i = 0; i < sizeof(p) / sizeof(uint8_t); i++) \
    { \
        p[i] = rand() % 256; \
    } \


static void generate_fake_uuid(char* output)
{
    uint8_t part1[4], part2[2], part3[2], part4[2], part5[6];
    FILL_UUID(part1); FILL_UUID(part2);
    FILL_UUID(part3); FILL_UUID(part4); FILL_UUID(part5);
    sprintf(
        output,
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        part1[0], part1[1], part1[2], part1[3],
        part2[0], part2[1],
        part3[0], part3[1],
        part4[0], part4[1],
        part5[0], part5[1], part5[2], part5[3], part5[4], part5[5]
    );
}

class RefCountObjectBase
{
public:
    size_t ref_count;

public:
    RefCountObjectBase() :
        ref_count(1)
    {
    }

    virtual ~RefCountObjectBase() {}

    size_t inc_ref()
    {
        return ++ref_count;
    }

    void dec_ref()
    {
        if (--ref_count == 0)
        {
            delete this;
        }
    }
};

class RefCountObjectFree : public RefCountObjectBase
{
public:
    gpointer object;

public:
    RefCountObjectFree(gpointer obj) :
        RefCountObjectBase(),
        object(obj)
    {
    }

    virtual ~RefCountObjectFree()
    {
        free(object);
    }
};

template<typename T>
class RefCountObjectDelete : public RefCountObjectBase
{
public:
    T* object;

public:
    RefCountObjectDelete(T* obj) :
        RefCountObjectBase(),
        object(obj)
    {
    }

    virtual ~RefCountObjectDelete()
    {
        delete object;
    }
};

class GCompareFuncComparer
{
private:
    GCompareDataFunc _comparer;
    gpointer _key_compare_data;

public:
    GCompareFuncComparer(GCompareDataFunc comparer, gpointer key_compare_data) :
        _comparer(comparer),
        _key_compare_data(_key_compare_data)
    {}

    bool operator()(const gpointer& a, const gpointer& b) const
    {
        // "_comparer" returns: negative value if a < b ; zero if a = b ; positive value if a > b;
        // std::map's comparator is essentially a 'less than' operator, so we do the appropriate
        // translation
        return _comparer(a, b, _key_compare_data) < 0;
    }
};

// This is a mock implementation of GLIB's map collection -
// GTree - using std::map<gpointer, gpointer>.
class CGTree
{
public:
    std::map<gpointer, gpointer, GCompareFuncComparer> _tree;
    GDestroyNotify _key_destroy_func;
    GDestroyNotify _value_destroy_func;

public:
    CGTree(GCompareDataFunc key_compare_func,
           gpointer key_compare_data,
           GDestroyNotify key_destroy_func,
           GDestroyNotify value_destroy_func) :
        _tree(GCompareFuncComparer(key_compare_func, key_compare_data)),
        _key_destroy_func(key_destroy_func),
        _value_destroy_func(value_destroy_func)
    {}

    ~CGTree()
    {
        if (_key_destroy_func != NULL || _value_destroy_func != NULL)
        {
            // invoke the destroy function for all keys and values
            for (const auto& it : _tree)
            {
                if (_key_destroy_func)
                {
                    _key_destroy_func(it.first);
                }

                if (_value_destroy_func)
                {
                    _value_destroy_func(it.second);
                }
            }
        }
    }

    void insert(gpointer key, gpointer value)
    {
        bool key_exists = _tree.find(key) != _tree.end();
        if (key_exists && _value_destroy_func)
        {
            // if a value with this key already exists in the tree then
            // destroy it by calling _value_destroy_func on it
            _value_destroy_func(_tree[key]);
        }

        _tree[key] = value;

        if (key_exists && _key_destroy_func)
        {
            // if a value with this key already exists in the tree then
            // destroy the new key passed in by calling _key_destroy_func on it
            _key_destroy_func(key);
        }
    }

    const gpointer lookup(const gpointer key) const
    {
        auto it = _tree.find(key);
        return it == _tree.end() ? NULL : it->second;
    }
};

/*
This is a mock implementation of GLib's GList data structure. Only
provides implementations for the following functions:
    g_list_find_custom
    g_list_foreach
    g_list_free
*/
class CGList
{
public:
    gpointer data;
    GList *next;
    GList *prev;

public:
    CGList(gpointer d) :
        data(d), next(NULL), prev(NULL)
    {}

    CGList(gpointer d, GList* n, GList* p) :
        data(d), next(n), prev(p)
    {}

    ~CGList()
    {
        // free the next node (it's destructor should
        // recursively free all other nodes)
        CGList* next = (CGList*)this->next;
        if (next)
        {
            delete next;
        }
    }

    void append(gpointer d)
    {
        // navigate to end of list
        GList* current = (GList*)this;
        while (current->next != NULL)
        {
            current = current->next;
        }

        // create new node and add to the end
        current->next = (GList*)(new CGList(d, NULL, current));
    }

    GList* find_custom(gconstpointer user_data, GCompareFunc func)
    {
        GList* current = (GList*)this;
        gint result = -1;
        do
        {
            result = func(current->data, user_data);
        } while (result != 0 && (current = current->next) != NULL);

        return current;
    }

    void foreach(GFunc func, gpointer user_data)
    {
        GList* current = (GList*)this;
        do
        {
            func(current->data, user_data);
            current = current->next;
        } while (current != NULL);
    }
};

class CGBytes
{
public:
    gpointer _data;
    gsize _size;

public:
    CGBytes(gconstpointer data, gsize size)
    {
        _data = NULL;
        _size = size;
        if (_size > 0)
        {
            _data = malloc(_size);
            memcpy(_data, data, _size);
        }
    }

    ~CGBytes()
    {
        free(_data);
    }

    gconstpointer get_data(gsize* size)
    {
        if (size != NULL)
        {
            *size = _size;
        }

        return _data;
    }
};

class CGVariant
{
public:
    enum Type
    {
        string,
        bytes,
        fixed_array
    };

    struct store_t
    {
        Type type;
        union
        {
            GString* str;
            RefCountObjectDelete<CGBytes>* bytes;
            CGBytes* fixed_array;
        } data;
    }store;

    CGVariant(const gchar* str)
    {
        store.type = CGVariant::string;
        store.data.str = g_string_new(str);
    }

    CGVariant(RefCountObjectDelete<CGBytes>* bytes)
    {
        store.type = CGVariant::bytes;
        store.data.bytes = bytes;
        bytes->inc_ref();
    }

    CGVariant(CGBytes* fixed_array)
    {
        store.type = CGVariant::fixed_array;
        store.data.fixed_array = fixed_array;
    }

    ~CGVariant()
    {
        if (store.type == CGVariant::string && store.data.str != NULL)
        {
            g_string_free(store.data.str, TRUE);
        }
        else if (store.type == CGVariant::bytes && store.data.bytes != NULL)
        {
            store.data.bytes->dec_ref();
        }
        else if (store.type == CGVariant::fixed_array && store.data.fixed_array != NULL)
        {
            delete store.data.fixed_array;
        }
    }
};

class DBUSInterface
{
public:
    std::string name;
    
    DBUSInterface(const std::string& iname) :
        name(iname)
    {}
};

class DBUSObject
{
public:
    std::string object_path;
    std::vector<DBUSInterface> interfaces_supported;

    DBUSObject(const std::string& path, const std::vector<DBUSInterface>& interfaces) :
        object_path(path),
        interfaces_supported(interfaces)
    {}
    
    DBUSObject(const DBUSObject& rhs) :
        object_path(rhs.object_path),
        interfaces_supported(rhs.interfaces_supported)
    {}
};

static DBUSObject g_dbus_objects[] =
{
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char001b",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0061/char0062",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0051/char0052/desc0054",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service004c",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF",
        {
            { "org.bluez.Device1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0027/char0028/desc002a",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0059",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char000f",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0008/char0009",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service002f/char0033",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF",
        {
            { "org.bluez.Device1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0051",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0027/char002b",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char000d",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service001f/char0025",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service001f/char0023",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0",
        {
            { "org.bluez.NetworkServer1" },
            { "org.bluez.Media1" },
            { "org.bluez.GattManager1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.Adapter1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service001f/char0020",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0027/char002d",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char0015",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service002f/char0030/desc0032",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char0019",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service003f",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char0017",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char0013",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service002f/char0035",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0061/char0066/desc0069",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0061/char0066/desc0068",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char0011",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service002f/char0030",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0047",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0047/char0048/desc004b",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0047/char0048/desc004a",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0008",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service001f/char0020/desc0022",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0037/char003d",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0037/char003b",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0008/char0009/desc000b",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service003f/char0045",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service002f",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service003f/char0043",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service003f/char0040",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0059/char005f",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0059/char005d",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0027/char0028",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0059/char005a",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0037",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF",
        {
            { "org.bluez.Device1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez",
        {
            { "org.bluez.ProfileManager1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.bluez.AgentManager1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0037/char0038/desc003a",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0059/char005a/desc005c",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0061/char0062/desc0065",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0061/char0062/desc0064",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service001f",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0051/char0057",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0037/char0038",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0051/char0055",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service004c/char004f",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service004c/char004d",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0051/char0052",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service000c/char001d",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service003f/char0040/desc0042",
        {
            { "org.bluez.GattDescriptor1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0027",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0061",
        {
            { "org.bluez.GattService1" },
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0047/char0048",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    },
    {
        "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0061/char0066",
        {
            { "org.freedesktop.DBus.Introspectable" },
            { "org.freedesktop.DBus.Properties" },
            { "org.bluez.GattCharacteristic1" }
        }
    }
};

class FakeObjectManager
{
public:
    GList* get_objects()
    {
        auto length = sizeof(g_dbus_objects) / sizeof(g_dbus_objects[0]);
        DBUSObject* bus_object = new DBUSObject(g_dbus_objects[0]);
        CGList* head = new CGList((gpointer)(new RefCountObjectDelete<DBUSObject>(bus_object)));
        for (size_t i = 1; i < length; i++)
        {
            bus_object = new DBUSObject(g_dbus_objects[i]);
            head->append((gpointer)(new RefCountObjectDelete<DBUSObject>(bus_object)));
        }

        return (GList*)head;
    }
};

class AsyncCallFinisherBase
{
public:
    size_t call_count;
    size_t when_shall_call_fail;

public:
    AsyncCallFinisherBase() :
        call_count(0),
        when_shall_call_fail(0)
    {}

    void reset()
    {
        call_count = when_shall_call_fail = 0;
    }
};

template<typename T>
class AsyncCallFinisherValueType : public AsyncCallFinisherBase
{
public:
    T async_call_finish(GAsyncResult* res, GError** error)
    {
        ++call_count;

        T result;
        if (when_shall_call_fail == call_count)
        {
            *error = g_error_new_literal(__LINE__, -1, "Whoops!");
            result = (T)0;
        }
        else
        {
            result = (T)1;
        }
        return result;
    }
};

template<typename T>
class AsyncCallFinisherRefType : public AsyncCallFinisherBase
{
public:
    T* async_call_finish(GAsyncResult* res, GError** error)
    {
        ++call_count;
        T* result;

        if (when_shall_call_fail == call_count)
        {
            *error = g_error_new_literal(__LINE__, -1, "Whoops!");
            result = NULL;
        }
        else
        {
            result = (T*)(new RefCountObjectFree((gpointer)(uintptr_t)malloc(1)));
        }

        return result;
    }
};

static AsyncCallFinisherRefType<GDBusConnection>        g_bus_finisher;
static AsyncCallFinisherRefType<bluezdevice>            g_bluez_device__proxy_new_finisher;
static AsyncCallFinisherValueType<gboolean>             g_bluez_device__call_connect_finisher;
static AsyncCallFinisherValueType<gboolean>             g_bluez_device__call_disconnect_finisher;
static AsyncCallFinisherRefType<bluezcharacteristic>    g_bluez_characteristic__proxy_new_finisher;
static AsyncCallFinisherValueType<gboolean>             g_bluez_characteristic__call_read_value_finisher;
static AsyncCallFinisherValueType<gboolean>             g_bluez_characteristic__call_write_value_finisher;

// The BLE config we use in all tests.
static BLE_DEVICE_CONFIG g_device_config =
{
    { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }, 0
};

static const char* g_bluez_device_path = "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF";

// We store the generated characteristic UUIDs in this vector so we can
// use them in tests.
std::vector<std::string> g_char_uuids;

TYPED_MOCK_CLASS(CBLEGATTIOMocks, CGlobalMock)
{
public:
    // memory
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_4(, GTree*, g_tree_new_full, GCompareDataFunc, key_compare_func, gpointer, key_compare_data, GDestroyNotify, key_destroy_func, GDestroyNotify, value_destroy_func)
        GTree* result2 = (GTree*)new CGTree(key_compare_func, key_compare_data, key_destroy_func, value_destroy_func);
    MOCK_METHOD_END(GTree*, result2);

    MOCK_STATIC_METHOD_1(, void, g_tree_unref, GTree*, tree)
        CGTree* gtree = (CGTree*)tree;
        delete gtree;
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, void, g_tree_insert, GTree*, tree, gpointer, key, gpointer, value)
        CGTree* gtree = (CGTree*)tree;
        gtree->insert(key, value);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, gpointer, g_tree_lookup, GTree*, tree, gconstpointer, key)
        CGTree* gtree = (CGTree*)tree;
        gpointer result2 = const_cast<gpointer>(gtree->lookup((const gpointer)key));
    MOCK_METHOD_END(gpointer, result2);

    MOCK_STATIC_METHOD_1(, GString*, g_string_new, const gchar*, init)
        GString* result2 = (GString*)malloc(sizeof(GString));
        result2->str = init != NULL ? g_strdup(init) : (gchar*)g_new0(gchar, 4);
        result2->len = init != NULL ? strlen(init) : 0;
        result2->allocated_len = init != NULL ? strlen(init) : 4;
    MOCK_METHOD_END(GString*, result2);

    MOCK_STATIC_METHOD_2(, gchar*, g_string_free, GString*, string, gboolean, free_segment)
        gchar* result2 = string->str;
        if (free_segment == TRUE)
        {
            g_free(result2);
            result2 = NULL;
        }
        free(string);
    MOCK_METHOD_END(gchar*, result2);

    MOCK_STATIC_METHOD_2(, GBytes*, g_bytes_new, gconstpointer, data, gsize, size)
        GBytes* result2 = (GBytes*)(
            new RefCountObjectDelete<CGBytes>(
                new CGBytes(data, size)
            )
        );
    MOCK_METHOD_END(GBytes*, result2);

    MOCK_STATIC_METHOD_2(, gconstpointer, g_bytes_get_data, GBytes*, bytes, gsize*, size)
        CGBytes* cgbytes = (CGBytes*)((RefCountObjectDelete<CGBytes>*)bytes)->object;
        gconstpointer result2 = cgbytes->get_data(size);
    MOCK_METHOD_END(gconstpointer, result2);

    MOCK_STATIC_METHOD_1(, void, g_bytes_unref, GBytes*, bytes)
        ((RefCountObjectDelete<CGBytes>*)bytes)->dec_ref();
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_5(, void, on_read_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context, BLEIO_GATT_RESULT, result2, const unsigned char*, buffer, size_t, size)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, void, on_write_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context, BLEIO_GATT_RESULT, result2)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void, on_disconnect_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, void, on_gatt_connect_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context, BLEIO_GATT_CONNECT_RESULT, connect_result)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_3(, GIO_ASYNCSEQ_HANDLE, GIO_Async_Seq_Create, gpointer, async_seq_context, GIO_ASYNCSEQ_ERROR_CALLBACK, error_callback, GIO_ASYNCSEQ_COMPLETE_CALLBACK, complete_callback)
        auto result2 = BASEIMPLEMENTATION::GIO_Async_Seq_Create(async_seq_context, error_callback, complete_callback);
    MOCK_METHOD_END(GIO_ASYNCSEQ_HANDLE, result2);

    MOCK_STATIC_METHOD_1(, void, GIO_Async_Seq_Destroy, GIO_ASYNCSEQ_HANDLE, async_seq_handle)
        BASEIMPLEMENTATION::GIO_Async_Seq_Destroy(async_seq_handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, gpointer, GIO_Async_Seq_GetContext, GIO_ASYNCSEQ_HANDLE, async_seq_handle)
        auto result2 = BASEIMPLEMENTATION::GIO_Async_Seq_GetContext(async_seq_handle);
    MOCK_METHOD_END(gpointer, result2);

    MOCK_STATIC_METHOD_1(, GIO_ASYNCSEQ_RESULT, GIO_Async_Seq_Run_Async, GIO_ASYNCSEQ_HANDLE, async_seq_handle)
        auto result2 = BASEIMPLEMENTATION::GIO_Async_Seq_Run_Async(async_seq_handle);
    MOCK_METHOD_END(GIO_ASYNCSEQ_RESULT, result2);

    MOCK_STATIC_METHOD_1(, void, g_object_unref, gpointer, object)
        RefCountObjectBase* refobj = (RefCountObjectBase*)object;
        refobj->dec_ref();
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_4(, void, g_bus_get, GBusType, bus_type, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, GDBusConnection*, g_bus_get_finish, GAsyncResult*, res, GError**, error)
        GDBusConnection* result2 = g_bus_finisher.async_call_finish(res, error);
    MOCK_METHOD_END(GDBusConnection*, result2);

    MOCK_STATIC_METHOD_10(, void, g_dbus_object_manager_client_new, GDBusConnection*, connection, GDBusObjectManagerClientFlags, flags, const gchar*, name, const gchar*, object_path, GDBusProxyTypeFunc, get_proxy_type_func, gpointer, get_proxy_type_user_data, GDestroyNotify, get_proxy_type_destroy_notify, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, GDBusObjectManager*, g_dbus_object_manager_client_new_finish, GAsyncResult*, res, GError**, error)
        GDBusObjectManager* result2 = (GDBusObjectManager*)new RefCountObjectDelete<FakeObjectManager>(
            new FakeObjectManager()
        );
    MOCK_METHOD_END(GDBusObjectManager*, result2);

    MOCK_STATIC_METHOD_7(, void, bluez_device__proxy_new, GDBusConnection*, connection, GDBusProxyFlags, flags, const gchar*, name, const gchar*, object_path, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, bluezdevice*, bluez_device__proxy_new_finish, GAsyncResult*, res, GError**, error)
        bluezdevice* result2 = g_bluez_device__proxy_new_finisher.async_call_finish(res, error);
    MOCK_METHOD_END(bluezdevice*, result2);

    MOCK_STATIC_METHOD_4(, void, bluez_device__call_connect, bluezdevice*, proxy, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, gboolean, bluez_device__call_connect_finish, bluezdevice*, proxy, GAsyncResult*, res, GError**, error)
        gboolean result2 = g_bluez_device__call_connect_finisher.async_call_finish(res, error);
    MOCK_METHOD_END(gboolean, result2);

    MOCK_STATIC_METHOD_4(, void, bluez_device__call_disconnect, bluezdevice*, proxy, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, gboolean, bluez_device__call_disconnect_finish, bluezdevice*, proxy, GAsyncResult*, res, GError**, error)
        gboolean result2 = g_bluez_device__call_disconnect_finisher.async_call_finish(res, error);
    MOCK_METHOD_END(gboolean, result2);

    MOCK_STATIC_METHOD_7(, void, bluez_characteristic__proxy_new, GDBusConnection*, connection, GDBusProxyFlags, flags, const gchar*, name, const gchar*, object_path, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, bluezcharacteristic*, bluez_characteristic__proxy_new_finish, GAsyncResult*, res, GError**, error)
        bluezcharacteristic* result2 = g_bluez_characteristic__proxy_new_finisher.async_call_finish(res, error);
    MOCK_METHOD_END(bluezcharacteristic*, result2);

    MOCK_STATIC_METHOD_4(, void, bluez_characteristic__call_read_value, bluezcharacteristic*, proxy, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_4(, gboolean, bluez_characteristic__call_read_value_finish, bluezcharacteristic*, proxy, gchar**, out_value, GAsyncResult*, res, GError**, error)
        gboolean result2 = g_bluez_characteristic__call_read_value_finisher.async_call_finish(res, error);
    MOCK_METHOD_END(gboolean, result2);

    MOCK_STATIC_METHOD_5(, void, bluez_characteristic__call_write_value, bluezcharacteristic*, proxy, const gchar *, arg_value, GCancellable *, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, gboolean, bluez_characteristic__call_write_value_finish, bluezcharacteristic*, proxy, GAsyncResult*, res, GError**, error)
        gboolean result2 = g_bluez_characteristic__call_write_value_finisher.async_call_finish(res, error);
    MOCK_METHOD_END(gboolean, result2);

    MOCK_STATIC_METHOD_1(, GList*, g_dbus_object_manager_get_objects, GDBusObjectManager*, manager)
        auto om = (FakeObjectManager*)((RefCountObjectDelete<FakeObjectManager>*)manager)->object;
        GList* result2 = om->get_objects();
    MOCK_METHOD_END(GList*, result2);

    MOCK_STATIC_METHOD_3(, GList*, g_list_find_custom, GList*, list, gconstpointer, data, GCompareFunc, func)
        GList* result2 = ((CGList*)list)->find_custom(data, func);
    MOCK_METHOD_END(GList*, result2);

    MOCK_STATIC_METHOD_3(, void, g_list_foreach, GList*, list, GFunc, func, gpointer, user_data)
        ((CGList*)list)->foreach(func, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, void, g_list_free, GList*, list)
        delete ((CGList*)list);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, const gchar*, g_dbus_proxy_get_object_path, GDBusProxy*, proxy)
        // this is called only for the device; so we return the device path
        const gchar* result2 = g_bluez_device_path;
    MOCK_METHOD_END(const gchar*, result2);

    MOCK_STATIC_METHOD_1(, const gchar*, g_dbus_proxy_get_interface_name, GDBusProxy*, proxy)
        DBUSInterface* dbus_interface = (DBUSInterface*)((RefCountObjectDelete<DBUSInterface>*)proxy)->object;
        const gchar* result2 = dbus_interface->name.c_str();
    MOCK_METHOD_END(const gchar*, result2);

    MOCK_STATIC_METHOD_2(, GVariant*, g_dbus_proxy_get_cached_property, GDBusProxy*, proxy, const gchar*, property_name)
        // this is called only to get the UUID of a characteristic; we return a fake one
        char uuid_str[40];
        generate_fake_uuid(uuid_str);
        g_char_uuids.push_back(uuid_str);
        GVariant* result2 = g_variant_new_string(uuid_str);
    MOCK_METHOD_END(GVariant*, result2);

    MOCK_STATIC_METHOD_8(, void, g_dbus_proxy_call, GDBusProxy*, proxy, const gchar*, method_name, GVariant*, parameters, GDBusCallFlags, flags, gint, timeout_msec, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
        g_variant_unref(parameters);
        callback(NULL, NULL, user_data);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, GVariant*, g_dbus_proxy_call_finish, GDBusProxy*, proxy, GAsyncResult*, res, GError**, error)
        *error = NULL;
        uint8_t fake_data[] = { 0xde, 0xad, 0xbe, 0xef };
        size_t size_data = sizeof(fake_data) / sizeof(uint8_t);
        GBytes* data = g_bytes_new(fake_data, size_data);
        GVariant* result2 = g_variant_new_from_bytes(
            G_VARIANT_TYPE_BYTESTRING,
            data,
            TRUE
        );
    MOCK_METHOD_END(GVariant*, result2);

    MOCK_STATIC_METHOD_1(, GVariant*, g_variant_new_string, const gchar*, string)
        GVariant* result2 = (GVariant*)(new RefCountObjectDelete<CGVariant>(
                new CGVariant(string)
            )
        );
    MOCK_METHOD_END(GVariant*, result2);

    MOCK_STATIC_METHOD_2(, const gchar*, g_variant_get_string, GVariant*, value, gsize*, length)
        CGVariant* var = ((RefCountObjectDelete<CGVariant>*)value)->object;
        const gchar* result2 = var->store.data.str->str;
        if (length != NULL)
        {
            *length = var->store.data.str->len;
        }
    MOCK_METHOD_END(const gchar*, result2);

    MOCK_STATIC_METHOD_3(, GVariant*, g_variant_new_from_bytes, const GVariantType*, type, GBytes*, bytes, gboolean, trusted)
        GVariant* result2 = (GVariant*)(new RefCountObjectDelete<CGVariant>(
                new CGVariant((RefCountObjectDelete<CGBytes>*)bytes)
            )
        );
    MOCK_METHOD_END(GVariant*, result2);

    MOCK_STATIC_METHOD_1(, GBytes*, g_variant_get_data_as_bytes, GVariant*, value)
        CGVariant* var = ((RefCountObjectDelete<CGVariant>*)value)->object;
        GBytes* result2 = (GBytes*)var->store.data.bytes;
    MOCK_METHOD_END(GBytes*, result2);

    MOCK_STATIC_METHOD_4(, GVariant*, g_variant_new_fixed_array, const GVariantType*, element_type, gconstpointer, elements, gsize, n_elements, gsize, element_size)
        GVariant* result2 = (GVariant*)(new RefCountObjectDelete<CGVariant>(
                new CGVariant(new CGBytes(elements, n_elements * element_size))
            )
        );
    MOCK_METHOD_END(GVariant*, result2);

    MOCK_STATIC_METHOD_2(, GVariant*, g_variant_new_tuple, GVariant * const *, children, gsize, n_children)
        GVariant* result2 = *children;
    MOCK_METHOD_END(GVariant*, result2);

    MOCK_STATIC_METHOD_1(, void, g_variant_unref, GVariant*, value)
        ((RefCountObjectDelete<CGVariant>*)value)->dec_ref();
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, const gchar*, g_dbus_object_get_object_path, GDBusObject*, object)
        DBUSObject* dbus_object = (DBUSObject*)((RefCountObjectDelete<DBUSObject>*)object)->object;
        const gchar* result2 = dbus_object->object_path.c_str();
    MOCK_METHOD_END(const gchar*, result2);

    MOCK_STATIC_METHOD_1(, GList*, g_dbus_object_get_interfaces, GDBusObject*, object)
        DBUSObject* dbus_object = (DBUSObject*)((RefCountObjectDelete<DBUSObject>*)object)->object;
        const auto& interfaces_supported = dbus_object->interfaces_supported;
        CGList* result2;
        if (interfaces_supported.empty())
        {
            result2 = NULL;
        }
        else
        {
            DBUSInterface* interface = new DBUSInterface(interfaces_supported[0]);
            result2 = new CGList((gpointer)(new RefCountObjectDelete<DBUSInterface>(interface)));
            for (size_t i = 1; i < interfaces_supported.size(); i++)
            {
                interface = new DBUSInterface(interfaces_supported[i]);
                result2->append((gpointer)(new RefCountObjectDelete<DBUSInterface>(interface)));
            }
        }
    MOCK_METHOD_END(GList*, (GList*)result2);
};

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, gballoc_free, void*, ptr);

DECLARE_GLOBAL_MOCK_METHOD_4(CBLEGATTIOMocks, , GTree*, g_tree_new_full, GCompareDataFunc, key_compare_func, gpointer, key_compare_data, GDestroyNotify, key_destroy_func, GDestroyNotify, value_destroy_func);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, g_tree_unref, GTree*, tree);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , void, g_tree_insert, GTree*, tree, gpointer, key, gpointer, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , gpointer, g_tree_lookup, GTree*, tree, gconstpointer, key);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , GList*, g_list_find_custom, GList*, list, gconstpointer, data, GCompareFunc, func);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , void, g_list_foreach, GList*, list, GFunc, func, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, g_list_free, GList*, list);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , GString*, g_string_new, const gchar*, init)
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , gchar*, g_string_free, GString*, string, gboolean, free_segment)

DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , GBytes*, g_bytes_new, gconstpointer, data, gsize, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , gconstpointer, g_bytes_get_data, GBytes*, bytes, gsize*, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, g_bytes_unref, GBytes*, bytes);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , GBytes*, g_variant_get_data_as_bytes, GVariant*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, g_variant_unref, GVariant*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , GVariant*, g_variant_new_string, const gchar*, string);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , const gchar*, g_variant_get_string, GVariant*, value, gsize*, length);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , GVariant*, g_variant_new_from_bytes, const GVariantType*, type, GBytes*, bytes, gboolean, trusted);
DECLARE_GLOBAL_MOCK_METHOD_4(CBLEGATTIOMocks, , GVariant*, g_variant_new_fixed_array, const GVariantType*, element_type, gconstpointer, elements, gsize, n_elements, gsize, element_size);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , GVariant*, g_variant_new_tuple, GVariant * const *, children, gsize, n_children);

DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , GIO_ASYNCSEQ_HANDLE, GIO_Async_Seq_Create, gpointer, async_seq_context, GIO_ASYNCSEQ_ERROR_CALLBACK, error_callback, GIO_ASYNCSEQ_COMPLETE_CALLBACK, complete_callback);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, GIO_Async_Seq_Destroy, GIO_ASYNCSEQ_HANDLE, async_seq_handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , gpointer, GIO_Async_Seq_GetContext, GIO_ASYNCSEQ_HANDLE, async_seq_handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , GIO_ASYNCSEQ_RESULT, GIO_Async_Seq_Run_Async, GIO_ASYNCSEQ_HANDLE, async_seq_handle);

DECLARE_GLOBAL_MOCK_METHOD_4(CBLEGATTIOMocks, , void, g_bus_get, GBusType, bus_type, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , GDBusConnection*, g_bus_get_finish, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_10(CBLEGATTIOMocks, , void, g_dbus_object_manager_client_new, GDBusConnection*, connection, GDBusObjectManagerClientFlags, flags, const gchar*, name, const gchar*, object_path, GDBusProxyTypeFunc, get_proxy_type_func, gpointer, get_proxy_type_user_data, GDestroyNotify, get_proxy_type_destroy_notify, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , GDBusObjectManager*, g_dbus_object_manager_client_new_finish, GAsyncResult*, res, GError**, error);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , GList*, g_dbus_object_manager_get_objects, GDBusObjectManager*, manager);

DECLARE_GLOBAL_MOCK_METHOD_7(CBLEGATTIOMocks, , void, bluez_device__proxy_new, GDBusConnection*, connection, GDBusProxyFlags, flags, const gchar*, name, const gchar*, object_path, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , bluezdevice*, bluez_device__proxy_new_finish, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_4(CBLEGATTIOMocks, , void, bluez_device__call_connect, bluezdevice*, proxy, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , gboolean, bluez_device__call_connect_finish, bluezdevice*, proxy, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_4(CBLEGATTIOMocks, , void, bluez_device__call_disconnect, bluezdevice*, proxy, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , gboolean, bluez_device__call_disconnect_finish, bluezdevice*, proxy, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_7(CBLEGATTIOMocks, , void, bluez_characteristic__proxy_new, GDBusConnection*, connection, GDBusProxyFlags, flags, const gchar*, name, const gchar*, object_path, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , bluezcharacteristic*, bluez_characteristic__proxy_new_finish, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_4(CBLEGATTIOMocks, , void, bluez_characteristic__call_read_value, bluezcharacteristic*, proxy, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_4(CBLEGATTIOMocks, , gboolean, bluez_characteristic__call_read_value_finish, bluezcharacteristic*, proxy, gchar**, out_value, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_5(CBLEGATTIOMocks, , void, bluez_characteristic__call_write_value, bluezcharacteristic*, proxy, const gchar *, arg_value, GCancellable *, cancellable, GAsyncReadyCallback, callback, gpointer, user_data);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , gboolean, bluez_characteristic__call_write_value_finish, bluezcharacteristic*, proxy, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , const gchar*, g_dbus_proxy_get_object_path, GDBusProxy*, proxy);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , const gchar*, g_dbus_proxy_get_interface_name, GDBusProxy*, proxy);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , const gchar*, g_dbus_object_get_object_path, GDBusObject*, object);
DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , GList*, g_dbus_object_get_interfaces, GDBusObject*, object);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , GVariant*, g_dbus_proxy_get_cached_property, GDBusProxy*, proxy, const gchar*, property_name);
DECLARE_GLOBAL_MOCK_METHOD_8(CBLEGATTIOMocks, , void, g_dbus_proxy_call, GDBusProxy*, proxy, const gchar*, method_name, GVariant*, parameters, GDBusCallFlags, flags, gint, timeout_msec, GCancellable*, cancellable, GAsyncReadyCallback, callback, gpointer, user_data)
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , GVariant*, g_dbus_proxy_call_finish, GDBusProxy*, proxy, GAsyncResult*, res, GError**, error);

DECLARE_GLOBAL_MOCK_METHOD_1(CBLEGATTIOMocks, , void, g_object_unref, gpointer, object);

DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , void, on_gatt_connect_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context, BLEIO_GATT_CONNECT_RESULT, connect_result);
DECLARE_GLOBAL_MOCK_METHOD_5(CBLEGATTIOMocks, , void, on_read_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context, BLEIO_GATT_RESULT, result2, const unsigned char*, buffer, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_3(CBLEGATTIOMocks, , void, on_write_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context, BLEIO_GATT_RESULT, result2);
DECLARE_GLOBAL_MOCK_METHOD_2(CBLEGATTIOMocks, , void, on_disconnect_complete, BLEIO_GATT_HANDLE, bleio_gatt_handle, void*, context);

/**
 * Poor man's mock for the var args function GIO_Async_Seq_Add.
 */
static bool g_was_GIO_Async_Seq_Add_called = false;
static size_t g_when_shall_GIO_Async_Seq_Add_fail = 0;
static size_t g_GIO_Async_Seq_Add_call = 0;
GIO_ASYNCSEQ_RESULT GIO_Async_Seq_Add(
    GIO_ASYNCSEQ_HANDLE async_seq_handle,
    gpointer callback_context,
    ...
)
{
    ++g_GIO_Async_Seq_Add_call;
    g_was_GIO_Async_Seq_Add_called = true;

    va_list args;
    va_start(args, callback_context);
    GIO_ASYNCSEQ_RESULT result;
    if (g_GIO_Async_Seq_Add_call == g_when_shall_GIO_Async_Seq_Add_fail)
    {
        result = GIO_ASYNCSEQ_ERROR;
    }
    else
    {
        result = BASEIMPLEMENTATION::GIO_Async_Seq_Addv(async_seq_handle, callback_context, args);
    }
    va_end(args);
    return result;
}

BEGIN_TEST_SUITE(ble_gatt_io_unittests)
    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (!MicroMockAcquireMutex(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        g_was_GIO_Async_Seq_Add_called = false;
        g_when_shall_GIO_Async_Seq_Add_fail = 0;
        g_GIO_Async_Seq_Add_call = 0;

        g_bus_finisher.reset();
        g_bluez_device__proxy_new_finisher.reset();
        g_bluez_device__call_connect_finisher.reset();
        g_bluez_device__call_disconnect_finisher.reset();
        g_bluez_characteristic__proxy_new_finisher.reset();
        g_bluez_characteristic__call_read_value_finisher.reset();
        g_bluez_characteristic__call_write_value_finisher.reset();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }

        g_char_uuids.clear();
    }

    /*Tests_SRS_BLEIO_GATT_13_003: [ BLEIO_gatt_create shall return NULL if config is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_create_returns_NULL_for_NULL_input)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act
        auto result = BLEIO_gatt_create(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_002: [ BLEIO_gatt_create shall return NULL when any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLEIO_gatt_create_returns_NULL_when_malloc_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = BLEIO_gatt_create(&g_device_config);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_002: [ BLEIO_gatt_create shall return NULL when any of the underlying platform calls fail. ]*/
    TEST_FUNCTION(BLEIO_gatt_create_returns_NULL_when_g_tree_new_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_new_full(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetFailReturn((GTree*)NULL);

        ///act
        auto result = BLEIO_gatt_create(&g_device_config);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NULL(result);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_001: [ BLEIO_gatt_create shall return a non-NULL handle on successful execution. ]*/
    TEST_FUNCTION(BLEIO_gatt_create_succeeds)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_new_full(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        ///act
        auto result = BLEIO_gatt_create(&g_device_config);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_NOT_NULL(result);

        ///cleanup
        BLEIO_gatt_destroy(result);
    }

    /*Tests_SRS_BLEIO_GATT_13_005: [ If bleio_gatt_handle is NULL BLEIO_gatt_destroy shall do nothing. ]*/
    TEST_FUNCTION(BLEIO_gatt_destroy_does_nothing_if_handle_is_NULL)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act
        BLEIO_gatt_destroy(NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_004: [ BLEIO_gatt_destroy shall free all resources associated with the handle. ]*/
    TEST_FUNCTION(BLEIO_gatt_destroy_frees_things_with_disconnected_handle)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_tree_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        ///act
        BLEIO_gatt_destroy(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_004: [ BLEIO_gatt_destroy shall free all resources associated with the handle. ]*/
    TEST_FUNCTION(BLEIO_gatt_destroy_frees_things_with_connected_handle)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // bus
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // object manager
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // device
        STRICT_EXPECTED_CALL(mocks, g_tree_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // this is the number of g_string_free calls we expect
        const size_t EXPECTED_STRING_FREES = (
            (sizeof(g_dbus_objects) / sizeof(g_dbus_objects[0])) + 5
        );
        for (size_t i = 0; i < EXPECTED_STRING_FREES; i++)
        {
            STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
                .IgnoreArgument(1);
        }

        ///act
        BLEIO_gatt_destroy(handle);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_010: [ If bleio_gatt_handle or on_bleio_gatt_connect_complete are NULL then BLEIO_gatt_connect shall return a non-zero value. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_returns_non_zero_with_NULL_handle)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act
        auto result = BLEIO_gatt_connect(NULL, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_010: [ If bleio_gatt_handle or on_bleio_gatt_connect_complete are NULL then BLEIO_gatt_connect shall return a non-zero value. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_returns_non_zero_with_NULL_callback)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_connect(handle, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_047: [ If when BLEIO_gatt_connect is called, there's another connection request already in progress or if an open connection already exists, then this API shall return a non-zero error code. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_returns_non_zero_when_in_invalid_state)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_009: [ If any of the underlying platform calls fail, BLEIO_gatt_connect shall return a non-zero value. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_returns_non_zero_when_malloc_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_009: [ If any of the underlying platform calls fail, BLEIO_gatt_connect shall return a non-zero value. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_returns_non_zero_when_GIO_Async_Seq_Create_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetFailReturn((GIO_ASYNCSEQ_HANDLE)NULL);

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_009: [ If any of the underlying platform calls fail, BLEIO_gatt_connect shall return a non-zero value. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_returns_non_zero_when_GIO_Async_Seq_Add_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        
        // since GIO_Async_Seq_Add has not been mocked because it's a varargs
        // function, we use this variable to control it's execution
        g_when_shall_GIO_Async_Seq_Add_fail = 1;

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_009: [ If any of the underlying platform calls fail, BLEIO_gatt_connect shall return a non-zero value. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_returns_non_zero_when_GIO_Async_Seq_Run_Async_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((GIO_ASYNCSEQ_RESULT)GIO_ASYNCSEQ_ERROR);

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_013: [The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation.]*/
    TEST_FUNCTION(BLEIO_gatt_connect_calls_callback_with_error_when_g_bus_get_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bus_get(G_BUS_TYPE_SYSTEM, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        g_bus_finisher.when_shall_call_fail = 1;
        STRICT_EXPECTED_CALL(mocks, g_bus_get_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, on_gatt_connect_complete(handle, NULL, BLEIO_GATT_CONNECT_ERROR));

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_013: [The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation.]*/
    TEST_FUNCTION(BLEIO_gatt_connect_calls_callback_with_error_when_g_dbus_object_manager_client_new_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bus_get(G_BUS_TYPE_SYSTEM, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_bus_get_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new(
            IGNORED_PTR_ARG,
            G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL, NULL, NULL, NULL,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG
            ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(9)
            .IgnoreArgument(10);

        GError* expected_error = g_error_new_literal(__LINE__, -1, "Whoops!");
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn((GDBusObjectManager*)NULL)
            .CopyOutArgumentBuffer(2, &expected_error, sizeof(GError*));
        STRICT_EXPECTED_CALL(mocks, on_gatt_connect_complete(handle, NULL, BLEIO_GATT_CONNECT_ERROR));

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_013: [The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation.]*/
    TEST_FUNCTION(BLEIO_gatt_connect_calls_callback_with_error_when_bluez_device__proxy_new_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bus_get(G_BUS_TYPE_SYSTEM, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_bus_get_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new(
            IGNORED_PTR_ARG,
            G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL, NULL, NULL, NULL,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG
            ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(9)
            .IgnoreArgument(10);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new(
            IGNORED_PTR_ARG,
            G_DBUS_PROXY_FLAGS_NONE,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG
            ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        g_bluez_device__proxy_new_finisher.when_shall_call_fail = 1;
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, on_gatt_connect_complete(handle, NULL, BLEIO_GATT_CONNECT_ERROR));
        STRICT_EXPECTED_CALL(mocks, g_string_new(NULL)); // create_device_proxy
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_013: [The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation.]*/
    TEST_FUNCTION(BLEIO_gatt_connect_calls_callback_with_error_when_bluez_device__call_connect_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bus_get(G_BUS_TYPE_SYSTEM, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_bus_get_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new(
            IGNORED_PTR_ARG,
            G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL, NULL, NULL, NULL,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG
            ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(9)
            .IgnoreArgument(10);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new(
            IGNORED_PTR_ARG,
            G_DBUS_PROXY_FLAGS_NONE,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG
            ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect(
            IGNORED_PTR_ARG,
            NULL,
            IGNORED_PTR_ARG,
            IGNORED_PTR_ARG
            ))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        g_bluez_device__call_connect_finisher.when_shall_call_fail = 1;
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, on_gatt_connect_complete(handle, NULL, BLEIO_GATT_CONNECT_ERROR));
        STRICT_EXPECTED_CALL(mocks, g_string_new(NULL)); // create_device_proxy
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_013: [The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation.]*/
    TEST_FUNCTION(BLEIO_gatt_connect_calls_callback_with_error_when_g_dbus_object_manager_get_objects_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bus_get(G_BUS_TYPE_SYSTEM, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_bus_get_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new(IGNORED_PTR_ARG, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, NULL, NULL, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(9)
            .IgnoreArgument(10);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_get_objects(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((GList*)NULL);
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, on_gatt_connect_complete(handle, NULL, BLEIO_GATT_CONNECT_ERROR));
        STRICT_EXPECTED_CALL(mocks, g_string_new(NULL)); // create_device_proxy
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_013: [The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation.]*/
    /*Tests_SRS_BLEIO_GATT_13_007: [ BLEIO_gatt_connect shall asynchronously attempt to open a connection with the BLE device. ]*/
    /*Tests_SRS_BLEIO_GATT_13_008: [ On initiating the connect successfully, BLEIO_gatt_connect shall return 0 (zero). ]*/
    /*Tests_SRS_BLEIO_GATT_13_011: [ When the connect operation to the device has been completed, the callback function pointed at by on_bleio_gatt_connect_complete shall be invoked. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_succeeds)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bus_get(G_BUS_TYPE_SYSTEM, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_bus_get_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new(IGNORED_PTR_ARG, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, NULL, NULL, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(9)
            .IgnoreArgument(10);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_get_objects(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_list_foreach(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_list_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(NULL)); // create_device_proxy
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);

        auto dbus_objects_length = sizeof(g_dbus_objects) / sizeof(g_dbus_objects[0]);
        std::string device_path = g_bluez_device_path;
        std::string characteristic_interface_name = "org.bluez.GattCharacteristic1";

        for (size_t i = 0; i < dbus_objects_length; i++)
        {
            // these APIs should get called once for every object on the bus
            STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_get_object_path(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, g_dbus_object_get_object_path(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            // if device_path is a prefix of the object_path then the
            // object is a child of the device
            if (device_path != g_dbus_objects[i].object_path &&
                std::equal(
                    device_path.begin(),
                    device_path.end(),
                    g_dbus_objects[i].object_path.begin()) == true)
            {
                // these APIs should get called once for all objects that are a child of
                // the device object
                STRICT_EXPECTED_CALL(mocks, g_dbus_object_get_interfaces(IGNORED_PTR_ARG))
                    .IgnoreArgument(1);

                STRICT_EXPECTED_CALL(mocks, g_list_find_custom(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                    .IgnoreArgument(1)
                    .IgnoreArgument(2)
                    .IgnoreArgument(3);

                // g_dbus_proxy_get_interface_name should get called for each interface on the
                // object till "org.bluez.GattCharacteristic1" is encountered
                const auto& interfaces_supported = g_dbus_objects[i].interfaces_supported;
                for (
                        std::vector<DBUSInterface>::const_iterator it =interfaces_supported.begin();
                        it != interfaces_supported.end();
                        ++it
                    )
                {
                    STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_get_interface_name(IGNORED_PTR_ARG))
                        .IgnoreArgument(1);
                    STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
                        .IgnoreArgument(1);

                    if (it->name == characteristic_interface_name)
                    {
                        // if the interface "org.bluez.GattCharacteristic1" is found then the following
                        // APIs should get called
                        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // process_object
                        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // get_characteristic_uuid
                        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // inside mock implementation of g_variant_new_string
                        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
                            .IgnoreArgument(1); // inside mock implementation of g_variant_unref
                        STRICT_EXPECTED_CALL(mocks, g_variant_get_string(IGNORED_PTR_ARG, NULL))
                            .IgnoreArgument(1); // get_characteristic_uuid
                        STRICT_EXPECTED_CALL(mocks, g_variant_new_string(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // inside mock implementation of g_dbus_proxy_get_cached_property
                        STRICT_EXPECTED_CALL(mocks, g_variant_unref(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // get_characteristic_uuid
                        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_get_cached_property(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                            .IgnoreArgument(1)
                            .IgnoreArgument(2);
                        STRICT_EXPECTED_CALL(mocks, g_tree_insert(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                            .IgnoreArgument(1)
                            .IgnoreArgument(2)
                            .IgnoreArgument(3);
                        break;
                    }
                }

                STRICT_EXPECTED_CALL(mocks, g_list_foreach(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
                    .IgnoreArgument(1)
                    .IgnoreArgument(2);
                STRICT_EXPECTED_CALL(mocks, g_list_free(IGNORED_PTR_ARG))
                    .IgnoreArgument(1);
            }
        }
        STRICT_EXPECTED_CALL(mocks, on_gatt_connect_complete(handle, NULL, BLEIO_GATT_CONNECT_OK));

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_013: [The connect_result parameter of the on_bleio_gatt_connect_complete callback shall indicate the status of the connect operation.]*/
    /*Tests_SRS_BLEIO_GATT_13_007: [ BLEIO_gatt_connect shall asynchronously attempt to open a connection with the BLE device. ]*/
    /*Tests_SRS_BLEIO_GATT_13_008: [ On initiating the connect successfully, BLEIO_gatt_connect shall return 0 (zero). ]*/
    /*Tests_SRS_BLEIO_GATT_13_011: [ When the connect operation to the device has been completed, the callback function pointed at by on_bleio_gatt_connect_complete shall be invoked. ]*/
    /*Tests_SRS_BLEIO_GATT_13_012: [ When on_bleio_gatt_connect_complete is invoked the value passed in callback_context to BLEIO_gatt_connect shall be passed along to on_bleio_gatt_connect_complete. ]*/
    TEST_FUNCTION(BLEIO_gatt_connect_passes_context)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bus_get(G_BUS_TYPE_SYSTEM, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_bus_get_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new(IGNORED_PTR_ARG, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, NULL, NULL, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(9)
            .IgnoreArgument(10);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_client_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_dbus_object_manager_get_objects(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_device__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_connect_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_list_foreach(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_list_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(NULL)); // create_device_proxy
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);

        auto dbus_objects_length = sizeof(g_dbus_objects) / sizeof(g_dbus_objects[0]);
        std::string device_path = g_bluez_device_path;
        std::string characteristic_interface_name = "org.bluez.GattCharacteristic1";

        for (size_t i = 0; i < dbus_objects_length; i++)
        {
            // these APIs should get called once for every object on the bus
            STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_get_object_path(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, g_dbus_object_get_object_path(IGNORED_PTR_ARG))
                .IgnoreArgument(1);
            STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
                .IgnoreArgument(1);

            // if device_path is a prefix of the object_path then the
            // object is a child of the device
            if (device_path != g_dbus_objects[i].object_path &&
                std::equal(
                    device_path.begin(),
                    device_path.end(),
                    g_dbus_objects[i].object_path.begin()) == true)
            {
                // these APIs should get called once for all objects that are a child of
                // the device object
                STRICT_EXPECTED_CALL(mocks, g_dbus_object_get_interfaces(IGNORED_PTR_ARG))
                    .IgnoreArgument(1);

                STRICT_EXPECTED_CALL(mocks, g_list_find_custom(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                    .IgnoreArgument(1)
                    .IgnoreArgument(2)
                    .IgnoreArgument(3);

                // g_dbus_proxy_get_interface_name should get called for each interface on the
                // object till "org.bluez.GattCharacteristic1" is encountered
                const auto& interfaces_supported = g_dbus_objects[i].interfaces_supported;
                for (
                        std::vector<DBUSInterface>::const_iterator it =interfaces_supported.begin();
                        it != interfaces_supported.end();
                        ++it
                    )
                {
                    STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_get_interface_name(IGNORED_PTR_ARG))
                        .IgnoreArgument(1);
                    STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
                        .IgnoreArgument(1);

                    if (it->name == characteristic_interface_name)
                    {
                        // if the interface "org.bluez.GattCharacteristic1" is found then the following
                        // APIs should get called
                        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // process_object
                        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // get_characteristic_uuid
                        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // inside mock implementation of g_variant_new_string
                        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
                            .IgnoreArgument(1); // inside mock implementation of g_variant_unref
                        STRICT_EXPECTED_CALL(mocks, g_variant_get_string(IGNORED_PTR_ARG, NULL))
                            .IgnoreArgument(1); // get_characteristic_uuid
                        STRICT_EXPECTED_CALL(mocks, g_variant_new_string(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // inside mock implementation of g_dbus_proxy_get_cached_property
                        STRICT_EXPECTED_CALL(mocks, g_variant_unref(IGNORED_PTR_ARG))
                            .IgnoreArgument(1); // get_characteristic_uuid
                        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_get_cached_property(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                            .IgnoreArgument(1)
                            .IgnoreArgument(2);
                        STRICT_EXPECTED_CALL(mocks, g_tree_insert(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                            .IgnoreArgument(1)
                            .IgnoreArgument(2)
                            .IgnoreArgument(3);
                        break;
                    }
                }

                STRICT_EXPECTED_CALL(mocks, g_list_foreach(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
                    .IgnoreArgument(1)
                    .IgnoreArgument(2);
                STRICT_EXPECTED_CALL(mocks, g_list_free(IGNORED_PTR_ARG))
                    .IgnoreArgument(1);
            }
        }
        STRICT_EXPECTED_CALL(mocks, on_gatt_connect_complete(handle, (void*)0x42, BLEIO_GATT_CONNECT_OK));

        ///act
        auto result = BLEIO_gatt_connect(handle, on_gatt_connect_complete, (void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_027: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if bleio_gatt_handle is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_for_NULL_input1)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(NULL, "fake_uuid", on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_051: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if ble_uuid is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_for_NULL_input2)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, NULL, on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_028: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if on_bleio_gatt_attrib_read_complete is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_for_NULL_input3)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid((BLEIO_GATT_HANDLE)0x42, "fake_uuid", NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_052: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if the object is not in a connected state. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_when_not_connected)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, "fake_uuid", on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_when_g_string_new_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new("fake_uuid"))
            .SetFailReturn((GString*)NULL);

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, "fake_uuid", on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_when_char_uuid_lookup_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new("fake_uuid"));
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, "fake_uuid", on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_when_malloc_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(g_char_uuids[0].c_str()));
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, g_char_uuids[0].c_str(), on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_when_GIO_Async_Seq_Create_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        auto char_uuid = g_char_uuids[0].c_str();
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetFailReturn((GIO_ASYNCSEQ_HANDLE)NULL);

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, char_uuid, on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_when_GIO_Async_Seq_Add_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        auto char_uuid = g_char_uuids[0].c_str();
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // since GIO_Async_Seq_Add has not been mocked because it's a varargs
        // function, we use this variable to control it's execution
        g_when_shall_GIO_Async_Seq_Add_fail = g_GIO_Async_Seq_Add_call + 1;

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, char_uuid, on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_returns_non_zero_when_GIO_Async_Seq_Run_Async_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        auto char_uuid = g_char_uuids[0].c_str();
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((GIO_ASYNCSEQ_RESULT)GIO_ASYNCSEQ_ERROR);

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, char_uuid, on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    /*Tests_SRS_BLEIO_GATT_13_035: [ When an error occurs asynchronously, the value BLEIO_GATT_ERROR shall be passed for the result parameter of the on_bleio_gatt_attrib_read_complete callback. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_calls_callback_with_error_when_bluez_characteristic__proxy_new_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        auto char_uuid = g_char_uuids[0].c_str();
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_error
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        g_bluez_characteristic__proxy_new_finisher.when_shall_call_fail = 1;
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, NULL, BLEIO_GATT_ERROR, NULL, 0));

        ///act
        (void)BLEIO_gatt_read_char_by_uuid(handle, char_uuid, on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_029: [ BLEIO_gatt_read_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    /*Tests_SRS_BLEIO_GATT_13_035: [ When an error occurs asynchronously, the value BLEIO_GATT_ERROR shall be passed for the result parameter of the on_bleio_gatt_attrib_read_complete callback. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_calls_callback_with_error_when_g_dbus_proxy_call_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        auto char_uuid = g_char_uuids[0].c_str();
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_error
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // read_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // read_characteristic_finish
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // free_context
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__call_read_value(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        
        GError* expected_error = g_error_new_literal(__LINE__, -1, "Whoops!");
        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_call_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .SetFailReturn((GVariant*)NULL)
            .CopyOutArgumentBuffer(3, &expected_error, sizeof(GError*));
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, NULL, BLEIO_GATT_ERROR, NULL, 0));

        ///act
        (void)BLEIO_gatt_read_char_by_uuid(handle, char_uuid, on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_030: [BLEIO_gatt_read_char_by_uuid shall return 0 (zero) if the read characteristic operation is successful.]*/
    /*Tests_SRS_BLEIO_GATT_13_032: [ BLEIO_gatt_read_char_by_uuid shall invoke on_bleio_gatt_attrib_read_complete when the read operation completes. ]*/
    /*Tests_SRS_BLEIO_GATT_13_031: [ BLEIO_gatt_read_char_by_uuid shall asynchronously initiate a read characteristic operation using the specified UUID. ]*/
    /*Tests_SRS_BLEIO_GATT_13_034: [ BLEIO_gatt_read_char_by_uuid, when successful, shall supply the data that has been read to the on_bleio_gatt_attrib_read_complete callback along with the value BLEIO_GATT_OK for the result parameter. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_succeeds)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        auto char_uuid = g_char_uuids[0].c_str();
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_variant_new_from_bytes(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_variant_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_variant_get_data_as_bytes(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_get_data(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // read_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // read_characteristic_finish
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_complete
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // free_context
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__call_read_value(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_call_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, NULL, BLEIO_GATT_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(4)
            .IgnoreArgument(5);

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, char_uuid, on_read_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_030: [BLEIO_gatt_read_char_by_uuid shall return 0 (zero) if the read characteristic operation is successful.]*/
    /*Tests_SRS_BLEIO_GATT_13_032: [ BLEIO_gatt_read_char_by_uuid shall invoke on_bleio_gatt_attrib_read_complete when the read operation completes. ]*/
    /*Tests_SRS_BLEIO_GATT_13_031: [ BLEIO_gatt_read_char_by_uuid shall asynchronously initiate a read characteristic operation using the specified UUID. ]*/
    /*Tests_SRS_BLEIO_GATT_13_034: [ BLEIO_gatt_read_char_by_uuid, when successful, shall supply the data that has been read to the on_bleio_gatt_attrib_read_complete callback along with the value BLEIO_GATT_OK for the result parameter. ]*/
    /*Tests_SRS_BLEIO_GATT_13_033: [ BLEIO_gatt_read_char_by_uuid shall pass the value of the callback_context parameter to on_bleio_gatt_attrib_read_complete as the context parameter when it is invoked. ]*/
    TEST_FUNCTION(BLEIO_gatt_read_char_by_uuid_passes_context)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        auto char_uuid = g_char_uuids[0].c_str();
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_variant_new_from_bytes(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, g_variant_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_variant_get_data_as_bytes(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_get_data(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // read_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // read_characteristic_finish
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_complete
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // free_context
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__call_read_value(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_call_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, on_read_complete(handle, (void*)0x42, BLEIO_GATT_OK, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(4)
            .IgnoreArgument(5);

        ///act
        auto result = BLEIO_gatt_read_char_by_uuid(handle, char_uuid, on_read_complete, (void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result == 0);
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_036: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if bleio_gatt_handle is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_for_NULL_input1)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(NULL, "fake_uuid", fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_048: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if ble_uuid is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_for_NULL_input2)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, NULL, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_037: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if on_bleio_gatt_attrib_write_complete is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_for_NULL_input3)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, "fake_uuid", fake_data, size_data, NULL, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_045: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if buffer is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_for_NULL_input4)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, "fake_uuid", NULL, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_046: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if size is equal to 0 (zero). ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_for_zero_size)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, "fake_uuid", fake_data, 0, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_053: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an active connection to the device does not exist. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_not_connected)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, "fake_uuid", fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_g_string_new_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((GString*)NULL);

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, "fake_uuid", fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_g_tree_lookup_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, "fake_uuid", fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_malloc_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_g_bytes_new_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .SetFailReturn((GBytes*)NULL);

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_GIO_Async_Seq_Create_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments()
            .SetFailReturn((GIO_ASYNCSEQ_HANDLE)NULL);

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_GIO_Async_Seq_Add_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // since GIO_Async_Seq_Add has not been mocked because it's a varargs
        // function, we use this variable to control it's execution
        g_when_shall_GIO_Async_Seq_Add_fail = g_GIO_Async_Seq_Add_call + 1;

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_038: [ BLEIO_gatt_write_char_by_uuid shall return a non-zero value if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_GIO_Async_Seq_Run_Async_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((GIO_ASYNCSEQ_RESULT)GIO_ASYNCSEQ_ERROR);

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result != 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_044: [ When an error occurs asynchronously, the value BLEIO_GATT_ERROR shall be passed for the result parameter of the on_bleio_gatt_attrib_write_complete callback. ] */
    /*Tests_SRS_BLEIO_GATT_13_040: [ BLEIO_gatt_write_char_by_uuid shall asynchronously initiate a write characteristic operation using the specified UUID. ]*/
    /*Tests_SRS_BLEIO_GATT_13_041: [ BLEIO_gatt_write_char_by_uuid shall invoke on_bleio_gatt_attrib_write_complete when the write operation completes. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_bluez_characteristic__proxy_new_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_error
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        g_bluez_characteristic__proxy_new_finisher.when_shall_call_fail = 1;
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, BLEIO_GATT_ERROR));

        ///act
        (void)BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_044: [ When an error occurs asynchronously, the value BLEIO_GATT_ERROR shall be passed for the result parameter of the on_bleio_gatt_attrib_write_complete callback. ] */
    /*Tests_SRS_BLEIO_GATT_13_040: [ BLEIO_gatt_write_char_by_uuid shall asynchronously initiate a write characteristic operation using the specified UUID. ]*/
    /*Tests_SRS_BLEIO_GATT_13_041: [ BLEIO_gatt_write_char_by_uuid shall invoke on_bleio_gatt_attrib_write_complete when the write operation completes. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_returns_non_zero_when_bluez_characteristic__call_write_value_finish_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_get_data(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_new_fixed_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, sizeof(unsigned char)))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_new_tuple(IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // from mock implementation of g_dbus_proxy_call
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // free_context
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // write_characteristic_finish
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_error
        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_call(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, G_DBUS_CALL_FLAGS_NONE, -1, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(7)
            .IgnoreArgument(8);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        g_bluez_characteristic__call_write_value_finisher.when_shall_call_fail = 1;
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__call_write_value_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, BLEIO_GATT_ERROR));

        ///act
        (void)BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_043: [BLEIO_gatt_write_char_by_uuid, when successful, shall supply the value BLEIO_GATT_OK for the result parameter.]*/
    /*Tests_SRS_BLEIO_GATT_13_040: [ BLEIO_gatt_write_char_by_uuid shall asynchronously initiate a write characteristic operation using the specified UUID. ]*/
    /*Tests_SRS_BLEIO_GATT_13_039: [ BLEIO_gatt_write_char_by_uuid shall return 0 (zero) if the write characteristic operation is successful. ]*/
    /*Tests_SRS_BLEIO_GATT_13_041: [ BLEIO_gatt_write_char_by_uuid shall invoke on_bleio_gatt_attrib_write_complete when the write operation completes. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_succeeds)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_get_data(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_new_fixed_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, sizeof(unsigned char)))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_new_tuple(IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // from mock implementation of g_dbus_proxy_call
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // free_context
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // write_characteristic_finish
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_complete
        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_call(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, G_DBUS_CALL_FLAGS_NONE, -1, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(7)
            .IgnoreArgument(8);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__call_write_value_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, NULL, BLEIO_GATT_OK));

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result == 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_043: [ BLEIO_gatt_write_char_by_uuid, when successful, shall supply the value BLEIO_GATT_OK for the result parameter.]*/
    /*Tests_SRS_BLEIO_GATT_13_040: [ BLEIO_gatt_write_char_by_uuid shall asynchronously initiate a write characteristic operation using the specified UUID. ]*/
    /*Tests_SRS_BLEIO_GATT_13_039: [ BLEIO_gatt_write_char_by_uuid shall return 0 (zero) if the write characteristic operation is successful. ]*/
    /*Tests_SRS_BLEIO_GATT_13_041: [ BLEIO_gatt_write_char_by_uuid shall invoke on_bleio_gatt_attrib_write_complete when the write operation completes. ]*/
    /*Tests_SRS_BLEIO_GATT_13_042: [ BLEIO_gatt_write_char_by_uuid shall pass the value of the callback_context parameter to on_bleio_gatt_attrib_write_complete as the context parameter when it is invoked. ]*/
    TEST_FUNCTION(BLEIO_gatt_write_char_by_uuid_passes_context)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        const char* char_uuid = g_char_uuids[0].c_str();
        unsigned char fake_data[] = "fake data";
        size_t size_data = sizeof(fake_data) / sizeof(unsigned char);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, g_string_new(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_tree_lookup(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_string_free(IGNORED_PTR_ARG, TRUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_new(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, g_bytes_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, g_bytes_get_data(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_new_fixed_array(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, sizeof(unsigned char)))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_new_tuple(IGNORED_PTR_ARG, 1))
            .IgnoreArgument(1); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, g_variant_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // from mock implementation of g_dbus_proxy_call
        STRICT_EXPECTED_CALL(mocks, g_object_unref(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // free_context
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_Run_Async(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // create_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // write_characteristic
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // write_characteristic_finish
        STRICT_EXPECTED_CALL(mocks, GIO_Async_Seq_GetContext(IGNORED_PTR_ARG))
            .IgnoreArgument(1); // on_sequence_complete
        STRICT_EXPECTED_CALL(mocks, g_dbus_proxy_call(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, G_DBUS_CALL_FLAGS_NONE, -1, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(7)
            .IgnoreArgument(8);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new(IGNORED_PTR_ARG, G_DBUS_PROXY_FLAGS_NONE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .IgnoreArgument(6)
            .IgnoreArgument(7);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__proxy_new_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, bluez_characteristic__call_write_value_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, on_write_complete(handle, (void*)0x42, BLEIO_GATT_OK));

        ///act
        auto result = BLEIO_gatt_write_char_by_uuid(handle, char_uuid, fake_data, size_data, on_write_complete, (void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();
        ASSERT_IS_TRUE(g_was_GIO_Async_Seq_Add_called);
        ASSERT_IS_TRUE(result == 0);

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_016: [ BLEIO_gatt_disconnect shall do nothing if bleio_gatt_handle is NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_disconnect_does_nothing_for_NULL_input1)
    {
        ///arrange
        CBLEGATTIOMocks mocks;

        ///act
        BLEIO_gatt_disconnect(NULL, on_disconnect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
    }

    /*Tests_SRS_BLEIO_GATT_13_015: [ BLEIO_gatt_disconnect shall do nothing if an open connection does not exist when it is called. ]*/
    TEST_FUNCTION(BLEIO_gatt_disconnect_does_nothing_when_not_connected)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        mocks.ResetAllCalls();

        ///act
        BLEIO_gatt_disconnect(handle, on_disconnect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_054: [ BLEIO_gatt_disconnect shall do nothing if an underlying platform call fails. ]*/
    TEST_FUNCTION(BLEIO_gatt_disconnect_does_nothing_when_malloc_fails)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .SetFailReturn((void*)NULL);

        ///act
        BLEIO_gatt_disconnect(handle, on_disconnect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_014: [ BLEIO_gatt_disconnect shall asynchronously disconnect from the BLE device if an open connection exists. ]*/
    /*Tests_SRS_BLEIO_GATT_13_050: [ When the disconnect operation has been completed, the callback function pointed at by on_bleio_gatt_disconnect_complete shall be invoked if it is not NULL. ]*/
    TEST_FUNCTION(BLEIO_gatt_disconnect_completes)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_disconnect(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_disconnect_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, on_disconnect_complete(handle, NULL));

        ///act
        BLEIO_gatt_disconnect(handle, on_disconnect_complete, NULL);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

    /*Tests_SRS_BLEIO_GATT_13_014: [ BLEIO_gatt_disconnect shall asynchronously disconnect from the BLE device if an open connection exists. ]*/
    /*Tests_SRS_BLEIO_GATT_13_050: [ When the disconnect operation has been completed, the callback function pointed at by on_bleio_gatt_disconnect_complete shall be invoked if it is not NULL. ]*/
    /*Tests_SRS_BLEIO_GATT_13_049: [ When on_bleio_gatt_disconnect_complete is invoked the value passed in callback_context to BLEIO_gatt_disconnect shall be passed along to on_bleio_gatt_disconnect_complete. ]*/
    TEST_FUNCTION(BLEIO_gatt_disconnect_passes_context)
    {
        ///arrange
        CBLEGATTIOMocks mocks;
        auto handle = BLEIO_gatt_create(&g_device_config);
        (void)BLEIO_gatt_connect(handle, on_gatt_connect_complete, NULL);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_disconnect(IGNORED_PTR_ARG, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, bluez_device__call_disconnect_finish(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, on_disconnect_complete(handle, (void*)0x42));

        ///act
        BLEIO_gatt_disconnect(handle, on_disconnect_complete, (void*)0x42);

        ///assert
        mocks.AssertActualAndExpectedCalls();

        ///cleanup
        BLEIO_gatt_destroy(handle);
    }

END_TEST_SUITE(ble_gatt_io_unittests)
