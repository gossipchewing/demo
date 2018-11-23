
Laravel 路由参数+注入模型实例过程解析
-----------------------------------

假定有如下路由定义及实际的请求url：
```
Route::delete('cart/{sku}', 'CartController@delete')->name('cart.delete');
@RequestUrl = /cart/17
```
有了以上的定义，请求时传递不同的参数值，就能在控制器方法中注入相应的 ProductSku 实例了：
```
class App\Models\ProductSku extends Illuminate\Database\Eloquent\Model {}

class CartController extends Controller 
{
 		public function delete(ProductSku $sku)
	    {
	        var_dump($sku->id);
	    }	
}
```

具体实现过程
-----------
### 应用启动大概流程：
+ new Application() --> $app->make(Kernel)
+ $kernel->handler() --> sendRequestThroughRouter() --> bootstrap(）
	+ bootstrapWith Bootstrappers：加载配置文件（.env, app/config/*.php）、注册|启动服务提供者...在这里完成。
	+ 服务提供者列表位于 ${config.php}.providers[];
	+ 启动服务提供者时，是调用各自的 boot 方法，路由服务提供者加载了路由配置文件(routes.php/*.php)，最终是通过调用 \App\Providers\RouteServiceProvider->map() 完成的。
+ send request through($middleware)：这里是通过全局中间件
+ dispatchToRouter() --> $this->router->dispatch($request)
+ 然后，就进入到路由分发部分啦~
	+ dispatchToRoute($request)
	+ runRoute(Request $request, Route $route)
	+ runRouteWithinStack($route, $request) // 这里就是通过路由中间件了！ 在这里会有一个 SubstituteBindings 的中间件

### 具体的实现就是在这个中间件中了：
$pipe = [Illuminate\Routing\Middleware\SubstituteBindings] —— 替换路由绑定
+ 在该中间件中调用 handler 方法处理：
```
/**
 * Handle an incoming request.
 *
 * @param  \Illuminate\Http\Request  $request
 * @param  \Closure  $next
 * @return mixed
 */
public function handle($request, Closure $next)
{
    $this->router->substituteBindings($route = $request->route());

    $this->router->substituteImplicitBindings($route);

    return $next($request);
}

class Illuminate\Routing\ImplicitRouteBinding
{
    /**
     * Resolve the implicit route bindings for the given route.
     *
     * @param  \Illuminate\Container\Container  $container
     * @param  \Illuminate\Routing\Route  $route
     * @return void
     */
    public static function resolveForRoute($container, $route)
    {
        $parameters = $route->parameters(); // 从路由对象中获得解析后的路由参数: ['sku' => 17]

        // （根据定义的路由得到对应的控制器和调用的方法，再）从方法的签名上获取签名参数，这里的 $parameter 是反射参数 ReflectionMethod 类的实例。 Illuminate\Database\Eloquent\Model 实现了 UrlRoutable 接口，因此支持路由参数绑定~
        foreach ($route->signatureParameters(UrlRoutable::class) as $parameter) {
        	// $parameterName 对应 'sku'
            if (! $parameterName = static::getParameterName($parameter->name, $parameters)) {
                continue;
            }

            $parameterValue = $parameters[$parameterName];  // 17

            if ($parameterValue instanceof UrlRoutable) {
                continue;
            }

            $instance = $container->make($parameter->getClass()->name); // 根据参数类型创建实例。

            if (! $model = $instance->resolveRouteBinding($parameterValue)) {
                throw (new ModelNotFoundException)->setModel(get_class($instance));
            }

            $route->setParameter($parameterName, $model);
        }
    }
}

// 我们自己的 Model 一般都是实现了下面的抽象类的。
abstract class Illuminate\Database\Eloquent\Model implements ArrayAccess, Arrayable, Jsonable, JsonSerializable, QueueableEntity, UrlRoutable
{
	// ......
	
	/**
     * Retrieve the model for a bound value.
     *
     * @param  mixed  $value
     * @return \Illuminate\Database\Eloquent\Model|null
     */
    public function resolveRouteBinding($value)
    {
        return $this->where($this->getRouteKeyName(), $value)->first();
    }

	/**
     * Get the route key for the model.
     *
     * @return string
     */
    public function getRouteKeyName()
    {
        return $this->getKeyName();
    }


    /**
     * Get the primary key for the model.
     *
     * @return string
     */
    public function getKeyName()
    {
        return $this->primaryKey;
    }

    /**
     * The primary key for the model.
     *
     * @var string
     */
    protected $primaryKey = 'id';

    // ......
}
```

显然，默认是拿 url 参数中的数值作为主键的值去数据库中查找对应的记录。 而且主键默认为 id。
可以在我们的 Model 中重写 $primary 字段；也可以重写 getKeyName 方法~
