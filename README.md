## 构建

- 使用CMake（GUI） 可直接构建

## 工程结构简述

- Core：核心模块
  - 3rdParty：第三方库
  - XHT：代码扫描工具
  - Test：模块功能的测试工程目录
  - XObject：基类封装

## 逻辑简述

### 基类XObject

xobject 作为所有对象的基类，提供序列化，反射等操作

### 反射

反射使用的库是Rttr，Rttr是一个优秀的C++反射库，它不仅仅能完成简单的类型信息反射，也提供了一套完整的反射处理接口：

- rttr::variant：用法类似stl::any，但更强大，提供了很多类型信息及转换接口（如 : 一个int类型的variant可直接导出为double，这里面就调用了int到double的转换接口，rttr里可以自定义variant的类型转换函数）
- 支持根据类型名称，创建对应实例，并提供三种创建模式（Type，Type\*，std::shared_ptr\<Type\*>）
- metadata 用于追加信息，利用该功能，可以为Property增加许多额外信息。



XHT用于代码扫描（参考QtMoc），通过宏标记来完成类型的自动注册，解决了RTTR需要手动编写注册函数问题。

> XHT看上去可有可无，自己写注册函数肯定也行，但还有更好的理由去使用XHT：
>
> - 不仅仅只有Rttr需要编写注册函数，涉及到序列化等其他为类型做附加的功能，都需要编写，目前的XObject，一个含两个简单类的h文件，XHT将生成近百行附加代码，这部分代码需要固定格式，自己写的话很累。
> - XHT不易出错，只需定义生成规则即可。
> - 原类改动时，XHT会自动同步附加代码的改动，而无需手动更改

#### 一些问题

##### 继承

Rttr使用模板注册反射类型，继承的处理是通过模板元实现 的（当子类调用父类函数时，会根据子类中定义的`using base_class_list = rttr::type_list<...>`，到父类中检索 ），因此必须把该定义写在 头文化中，才能保证该定义对子类可见，这就需要在反射标记时，需要再次手动指定父类，就像是这样：

```C++
class Base : public XObject {
	XENTRY(XObject) 					//必须在这里指定父类，否则Rttr中无法确定继承关系
}
```

##### 退化指针

在Rttr中，无法用父类的指针去调用子类的方法，比如下面的代码：

```c++
XObject* x = new Base;					//Base中包含print方法
rttr::invoke(x,"print");				//调用失败，原因是Rttr无法判断x的真实类型
```

XObject在宏`XEntry()`中添加了如下定义：

```c++
virtual rttr::instance instance() override { return *this; } 
```

这为XObject的子类提供了一个虚函数，用于获取rttr的实例，这就避免了上面指针的问题，调用看起来像是这样的：

```C++
XObject* x = new Base;
rttr::invoke(x->instance(),"print");	//调用成功
```

##### 注册时机

官方给的做法是静态注册，其原理可以简化如下：

```c++
//此位于CPP中
struct Register{
	Register(){
		//此处进行RTTR的类型注册
	}
};
static Register register;
```

说明：RTTR通过在CPP中创建一个结构体，把注册函数写在结构体的构造函数中，然后再创建一个静态实例，从而完成类型的注册。

这样做主要存在以下问题：

- 注册的顺序不容易控制。
- 一次性注册所有反射类型容易导致程序启动时的卡顿。

XObject中为了解决该问题，增加了**XMetaObject**类，它的结构同上面的`Register`相似（即Rttr的注册代码位于其构造函数中），不同的是，我们并没有直接在cpp中创建XMetaObject的静态实例，而是通过如下的方式实现：

```C++
class XObject{
	static XMetaObject* staticMetaObject(){
        static XMetaObject instance;
        return &instance;
    }
}
```

当初次访问`staticMetaObject()`函数时，才会创建里面的静态实例，在加上XObject使用XMetaObject作为反射数据的唯一入口，所以在使用该类型的反射接口时，会自动进行Rttr注册。

这里还有个细节：

当使用子类反射接口时调用父类接口时，需要保证父类已经注册。

XObject中为了完成该操作，在子类MetaObject的构造函数中，会调用一次父类的`staticMetaObject`保证父类提前注册

> ### 元对象
>
> 上面提到了XMetaObject，这里简单说下XObject是如何使用它作为类型唯一的反射入口。
>
> 使用反射，主要是为了以下用途：
>
> - 读写Property、获取Property信息（类型，元数据...）
> - 调用Function、获取Function信息（参数信息...）
> - 根据类型名称创建实例
>
> 而XMetaObject是以类型相关，实例无关的，通过XMetaObject仅仅是为了获取反射数据，它的部分代码如下
>
> ```C++
> struct XMetaObject {
> public:
> 	XMetaObject();
> 	XObject* newInstance(std::vector<rttr::argument> args = {});	//创建实例
> 	rttr::property getProperty(std::string name);					//获取名为name的property
> 	rttr::array_range<rttr::property> getProperties();				//获取该类型的所有property
> 	rttr::method getMethod(std::string name);						//获取名为name的method
> 	rttr::array_range<rttr::method> getMethods();					//获取该类型的所有method
> };
> ```
>
> 而调用函数是与实例相关的，想要调用函数就必须提供一个实例，这个操作看起来像是这样：
>
> ```c++
> Base* base = new Base;
> XMetaObject* meta = Base::staticMetaObject();			//获取Base的静态元对象
> rttr::method method = meta->getMethod("print");			//获取Base的print函数的信息
> method.invoke(base);  									//调用时需要提供实例
> ```
>
> 众所周知，弱化的指针类型是无法调用实际类型的静态函数，即：
>
> ```c++
> XObejct *x = new Base;
> x->staticMetaObject();		//此时将会调用XObject::staticMetaObject()而非Base::staticMetaObject()
> ```
>
> 为了解决这个问题，宏`XEntry()`中增加了如下定义：
>
> ```C++
> virtual XMetaObject* metaObject() override { return staticMetaObject(); }
> ```
>
> 另外，在Rttr中根据类型名称在类型池中进行搜索的代价是高昂的，为了解决这个问题，XMetaObject提供了额外的虚函数用于MetaObject到rttr::type的绑定：
>
> ```c++
> virtual rttr::type getRttrType() { return rttr::type::get<void>(); };	
> ```
>
> XHT生成Rttr的注册函数时，也会生成该函数的定义，该函数返回一个已经确定了的rttr::type
>
> 根据元对象的接口，XObject也提供了一些与实例相关的反射接口：
>
> ```C++
> bool setProperty(std::string name, rttr::argument var);		//设置属性
> rttr::variant getProperty(std::string name);				//获取属性
> rttr::variant invoke(rttr::string_view name, std::vector<rttr::argument> args = {});		//调用函数
> ```

### 序列化

如果手动编写序列化函数的话，序列化并不困难，但如果想要自动序列化，就得面临以下难题：

- 自定义类型的序列化
- 复杂类型（容器）的序列化
- 指针的反序列化
- 反序列化的运行时错误检查

对于不同的序列化目的有着不同的处理方式：

- 二进制序列化拥有最好的性能及内存，但其内容难以阅读。
- 非二进制序列化（如xml，json，cbor等），拥有优雅的存储结构，可读性很强，但性能及内存的损耗相对较高。

#### Binary

二进制序列化使用的库是[BitSery](https://github.com/fraillt/bitsery)，它通过了一些模板操作来支持复杂类型（容器）的序列化

想要一个类型支持BitSery序列化，只需提供模板函数`serialize(Serialize& s, type& o)`：

```C++
struct MyStruct {
    uint32_t i;
    std::vector<float> fs;
};

template <typename Serialize>
void serialize(Serialize& s, MyStruct& o) {	 //该模板函数即可作为序列化函数，又可作为反序列函数
    s(o.i);
    s(o.fs);
}
```

要想序列化该对象，只需：

```C++
using SerializeBuffer = std::vector<uint8_t>;		//定义Buffer的类型
using OutputAdapter = bitsery::OutputBufferAdapter<SerializeBuffer>;
using InputAdapter = bitsery::InputBufferAdapter<SerializeBuffer>;
using Serializer = bitsery::Serializer<OutputAdapter>;
using Deserializer = bitsery::Deserializer<InputAdapter>;

void main(){
    MyStruct myStruct;
    SerializeBuffer buffer;
    
    //序列化，将myStruct输出到buffer中
    bitsery::quickSerialization(OutputAdapter(buffer), myStruct); 
    
    //反序列化，从buffer中读取数据输入到myStruct中
    bitsery::quickDeserialization(InputAdapter(buffer), myStruct);   	
}
```

##### 绕过模板

XObject中为了避免模板，将序列化与反序列化函数拆分，在宏**XENTRY()**中添加了如下定义：

```c++
virtual void __intrusive_deserialize(Deserializer& deserializer) override; 
virtual void __intrusive_serialize(Serializer& serializer) override; 
```

其函数的实现由XHT根据Property生成，此外，这里还会调用父类的序列化函数。

> 这里使用了模板转接Bitsery API，存在的问题是：通过模板无法判断类型的继承关系
>
> 好在实现Rttr继承时添加了一些定义，利用这些定义可以轻松判断某个类型是否是XObject的子类：
>
> ```C++
> template<typename T, typename std::enable_if< std::is_class<T>::value&& rttr::detail::has_base_class_list<T>::value>::type* = nullptr>
> void serialize(Serializer& serializer, T& ptr) {
> 	ptr.__intrusive_serialize(serializer);
> }
> 
> template<typename T, typename std::enable_if<std::is_class<T>::value&& rttr::detail::has_base_class_list<T>::value>::type* = nullptr>
> void serialize(Deserializer& deserializer, T& ptr) {
> 	ptr.__intrusive_deserialize(deserializer);
> }
> ```

##### 指针特化

由于Bitsery并不支持指针的序列化，因此要专门编写指针的序列化模板。

指针类型和原类型有两点不同：

- 指针可以为空，nullptr不能进行序列化
- 指针反序列化时，可能需要新建实例

为了解决这两个问题，采用了如下的解决方式：

- 序列化时写入指针的类型，空指针则写入空类型，反序列化时，一定会先读取类型，类型不为空，则继续序列化
- 由于序列化时，写入了指针类型，所以可以通过RTTR根据类型名称新建实例，但需要保证该类型注册了一个无参构造函数。

但对XObject来说，还需要做更多—— 对于`XObject* x = new Base();`中的x来说，并不能简单使用`std::remove_pointer_t<T>::type`来获取原类型，所以对于XObject的类型，需要从实例的metaObject对象中获取，其核心代码如下：

```c++
template<typename T, typename std::enable_if<std::is_pointer<T>::value && !rttr::detail::has_base_class_list<rttr::detail::raw_type_t<T>>::value>::type* = nullptr>
void serialize(Serializer& writer, T& ptr) {
	std::string typeName;
	if (ptr != nullptr) {
		rttr::type type = rttr::type::get<rttr::detail::raw_type_t<T>::type>();		//根据指针类型获取原始类型
		typeName = type.get_raw_type().get_name().to_string();
	}
	writer(typeName);					//写入类型
	if (ptr != nullptr) {
		writer(*ptr);					//如果指针不为空，写入该指针所指数据
	}
}

template<typename T, typename std::enable_if<std::is_pointer<T>::value&& rttr::detail::has_base_class_list<rttr::detail::raw_type_t<T>>::value>::type* = nullptr>
void serialize(Serializer& writer, T& ptr) {
	std::string typeName;
	if (ptr != nullptr) {
		rttr::type type = ptr->metaObject()->getRttrType();							//从metaObject中获取类型
		typeName = type.get_raw_type().get_name().to_string();
	}
	writer(typeName);					//写入类型
	if (ptr != nullptr) {
		writer(*ptr);					//如果指针不为空，写入该指针所指数据
	}	
}

template<typename T, typename std::enable_if<std::is_pointer<T>::value>::type* = nullptr>
void serialize(Deserializer& reader, T& ptr) {
	std::string typeName;				
	reader(typeName);					//读取类型
	if (typeName.empty())				//如果类型为空，则直接返回
		return;
	if (ptr != nullptr) {				//如果反序列所指的对象已不为空，则直接对其进行反序列化
		reader(*ptr);
		return;
	}
	rttr::type type = rttr::type::get_by_name(typeName);		//获取Rttr类型
	if (!type.is_valid())
		return;
	rttr::variant var = type.create();							//利用Rttr创建实例
	if (!var.can_convert<T>()) {								
		return;
	}
	ptr = var.get_value<T>();									//如果实例合法，则对其进行反序列化
	reader(*ptr);
}
```

##### Include优化

Bitsery是一个 **Header - Only**库，为了不将所有内容都引入到**XObject.h**中，这里的做法是把核心的定义写入到**SerializationDefine.h**中，XObject只包含这个文件；而对于其他的代码，在**SerializationBriefSyntax.h**中被引入，XHT生成附加代码时会包含该文件。



#### Json And Cbor

这里使用的库是 [nlohmann json](https://github.com/nlohmann/json.git)，这里的实现就有些草率，只完成了反序列而没有反序列化，只能用作调试。而序列化函数也是通过XHT生成的，理所应当的，**XENTRY()**中增加了新的定义：

```c++
virtual void __intrusive_to_json(nlohmann::json& json) const override;
```

其处理方式也与二进制序列化相似。

json序列化一开始是想通过反射来做，因为我之前在Qt中也是通过反射进行json序列化的，可当我真正去实现的时候，才发现它原没有这么简单：

通过rttr，能读取到一个rttr::variant 数据，我要怎么做才能将其自动转换成json里的数据呢？貌似我只能这样：

```C++
rttr::variant property;
if(var.isType<int>()){
	json[propertyName] = property.get_value<int>();
}
else if(var.isType<double>()){
	json[propertyName] = property.get_value<double>();
}
else if(...){
	...
}
...
```

> 曲线...救国...？

我之前一直以为：Qt中对QVariant进行IO会自动转接到对应类型的序列化函数上。

而rttr::variant并不支持这样做，所以我去查阅QVariant的做法，想要填补这部分缺陷。

发现它的做法其实也是这样的，只不过用了个Map存储函数：

```c++
rttr::variant property;
json[propertyName] = FuncMap[property.typeId()](property);
```

对于自定义类型，Qt声明需要使用宏**Q_DECLARE_METATYPE** 进行修饰：

```C++
struct CustomType{};
Q_DECLARE_METATYPE(CustomType)  //该宏会注册meta type，如果该类型存在序列化函数，则会将函数指针与类型id进行绑定。
```

RTTR的注册并不支持这样的操作，因此这里就暂时还是用XHT生成json的序列化函数。

### 编辑器

Core模块完全不依赖于Editor模块，而Editor模块则是利用Core中的反射数据制作编辑器

我之前写过很多编辑器，做过很多尝试，主要经历了以下的阶段：

- **编辑器为核心**

  - 早期的开发目的也很简单：就是需要一个什么什么样的编辑器，来完成什么样的效果，所以就直接开始设计编辑器的UI和逻辑，编辑器直接调用效果类的方法进行效果编辑，有时候还得专门让效果类提供一些特定API给编辑器。这样做到最后，原本的效果类改的面目全非，编辑器和效果类的代码也写的到处都是，耦合非常严重，最后编写导入导出接口的时候还得异常小心。

- **Property为核心的类型编辑器**：到这之前还有很多个阶段过渡，但没有再说的意义。你可以通过以下特征做一些了解：

  - property为编辑目标，且拥有逻辑一致的读写接口

    > 编写效果类时，应预留可编辑的property，并为之提供相应的读写接口，并在使用这些property的时候，保证它们的同步状况，这样才能正确利用编辑器序列化产生的数据。

  - 编辑器部件与数据类型一一对应

    > 即：
    >
    > int类型有int的编辑部件、曲线类有曲线的编辑部件、树结构有树的编辑部件...
    >
    > 这些部件的粒度都很细，完整的编辑器都是由这些小部件组装而来。

