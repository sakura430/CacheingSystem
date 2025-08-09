1.weak_ptr will be initialized to nullptr by its constructor,but shared_ptr must initialized explicitly.

2. explicit Fre_List(int freq);

.Explanation:

explicit prevents the compiler from using this constructor for implicit conversions or copy-initialization.
Without explicit, code like Fre_List<int, int> fl = 5; would compile, creating a Fre_List with freq=5 by accident.
With explicit, only direct initialization is allowed: Fre_List<int, int> fl(5);
Summary:
Use explicit to avoid unintended implicit conversions and make your code safer and clearer, especially for single-argument constructors.