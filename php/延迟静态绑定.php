<?php
/**
 * 【准确说，后期静态绑定工作原理是存储了在上一个“非转发调用”（non-forwarding call）的类名。】
 *
 ** 也就是说 static 真正绑定的那个类就是上一个非转发调用处所用的类：
 ** （1）当进行静态方法调用时，该类名即为明确指定的那个（通常在 :: 运算符左侧部分）；
 ** （2）当进行非静态方法调用时，即为该对象所属的类。
 *
 * 所谓的“转发调用”（forwarding call）指的是通过以下几种方式进行的静态调用：self::，parent::，static:: 以及 forward_static_call()。
 * 
 * 可用 get_called_class() 函数来得到被调用的方法所在的类名，static:: 则指出了其范围。
 *
 *********************
 * forward_static_call — Call a static method
 * 
 * mixed forward_static_call ( callable $function [, mixed $parameter [, mixed $... ]] )
 * 它的功能类似 call_user_func，不过，它使用的延迟静态绑定(late static binding)。 
 * static 只能用在类中，同样的，此方法也只能在类中使用。
 */
<?php
// static 后期静态绑定的工作原理是存储了上一个非转发调用（non-forwarding call）的类名。
class A {
    public static function foo() {
        static::who(); // static 反向查找第一个非转发调用的调用点。
    }

    public static function who() {
        echo __CLASS__."[1]\n";
    }
}

class B extends A {
    public static function test() {
		// A[1]
        A::foo(); // A:: 不是转发调用。找到此处即停止。
		echo PHP_EOL;

		// C[3]
        parent::foo(); // parent:: 是转发调用，还要继续往上找。
		// C[3]
        self::foo(); // self:: 是转发调用 ...
		echo PHP_EOL;
		
		// C[3]
		forward_static_call([__CLASS__, 'foo']); // forward_static_call 本身就是转发调用，所以，要继续找！
		// B[2]
		call_user_func([__CLASS__, 'foo']); // 相当于调用： B::foo(); —— 所以，这里不是转发调用
		
		echo PHP_EOL;
		// C[3]
		forward_static_call(['self', 'foo']); // forward_static_call 也是转发调用 ...
		// C[3]
		call_user_func(['self', 'foo']); // 相当于调用： self::foo(); —— 所以，这里依然是转发调用！
    }

    public static function who() {
        echo __CLASS__."[2]\n";
    }
}
class C extends B {
	const VAL = '__C__';
	
    public static function who() {
        echo __CLASS__."[3]\n";
    }
}

C::test();

/**
 * result:
 *  	A[1]
 *  
 *  	C[3]
 *  	C[3]
 *  
 *  	C[3]
 *  	B[2]
 *  
 *  	C[3]
 *  	C[3]
 */
 //////////////////////////////////////////////////////////////////////////////////////////////////////

class A
{
    const NAME = 'A';
    public static function test() {
        $args = func_get_args();
        echo static::NAME, " ".join(',', $args)." \n";
    }
}

class B extends A
{
    const NAME = 'B';

    public static function test() {
        echo self::NAME, "\n";
		
		// 尽管调用的是 A::test()，但是 forward_static_call 本身是转发调用形式。所以，要确定具体的 static 指向，还要继续往上找。
        forward_static_call(array('A', 'test'), 'more', 'args');	// B more,args
		// 相当于直接调用 A::test() 
        call_user_func(array('A', 'test'), 'more', 'args');			// A more,args
		
        forward_static_call( 'test', 'other', 'args');	// C other,args
        call_user_func( 'test', 'other', 'args');		// C other,args
    }
}

function test() {
	$args = func_get_args();
	echo "C ".join(',', $args)." \n";
}

// PHP Fatal error:  Uncaught Error: Cannot call forward_static_call() when no class scope is active 
// forward_static_call( 'test', 'other', 'args'); // 只能在类中使用该方法
call_user_func( 'test', 'other', 'args');		// C other,args

B::test('foo'); 
/**
	B
	
	B more,args
	A more,args
	
	C other,args
	C other,args
*/
