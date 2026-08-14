#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kennedy Coelho Nolasco");
MODULE_DESCRIPTION("Driver de acesso ao WeatherStation (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100     // Tamanho máxima de linha de resposta
#define RESPONSE_TIMEOUT 5000 // Tempo (em ms) que deve aguardar por uma resposta

/* Declaração de funções de controle */

/* Executa quando o dispositivo é conectado no USB. */
static int usb_probe(struct usb_interface *ifce, const struct usb_device_id *id);

/* Executa quando o dispositivo é removido. */
static void usb_disconnect(struct usb_interface *ifce);

/* Envia um comando via serial USB para o ESP32 e retorna a mensagem de resposta do ESP. Caso não obtenha a resposta desejada, retorna NULL. */
static char *usb_send_cmd(char *cmd);

/* Executado quando o arquivo /sys/kernel/weatherstation/{weather} é lido. */
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);

/* Executado quando há uma tentativa de escrita no arquivo /sys/kernel/weatherstation/{weather}. */
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

/* Buffers de entrada e saída*/
static char recv_line[MAX_RECV_LINE];            // Armazena dados vindos da USB até receber um caractere de nova linha '\n'
static struct usb_device *weatherstation_device; // Referência para o dispositivo USB
static uint usb_in, usb_out;                     // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;     // Buffers de entrada e saída da USB
static int usb_max_size;                         // Tamanho máximo de uma mensagem USB

/* Variáveis para criação de atributos no sistema de arquivos */
static struct kobj_attribute weather_attribute = __ATTR(weather, S_IRUGO, attr_show, attr_store);
static struct attribute *attrs[] = {&weather_attribute.attr, NULL};
static struct attribute_group attr_group = {.attrs = attrs};
static struct kobject *sys_obj;

/* Registra o CP2102 (Chip USB-Serial do ESP32) no USB-Core do kernel */
#define VENDOR_ID 0x10C4                                                                  /* VendorID  do CP2102 */
#define PRODUCT_ID 0xEA60                                                                 /* ProductID do CP2102 */
static const struct usb_device_id id_table[] = {{USB_DEVICE(VENDOR_ID, PRODUCT_ID)}, {}}; // Tabela de dispositivos
MODULE_DEVICE_TABLE(usb, id_table);
bool ignore = true;

/* Cria e registra o driver do weatherstation no kernel */
static struct usb_driver weatherstation_driver = {
    .name = "weatherstation",     // Nome do driver
    .probe = usb_probe,           // Executado quando o dispositivo é conectado na USB
    .disconnect = usb_disconnect, // Executado quando o dispositivo é desconectado na USB
    .id_table = id_table,         // Tabela com o VendorID e ProductID do dispositivo
};
module_usb_driver(weatherstation_driver);

/* Implementação das funções declaradas no início do código. */

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;

    printk(KERN_INFO "WeatherStation: Dispositivo conectado.\n");

    // Cria arquivos do /sys/kernel/weatherstation/*
    sys_obj = kobject_create_and_add("weatherstation", kernel_kobj);
    ignore = sysfs_create_group(sys_obj, &attr_group); // AQUI

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    weatherstation_device = interface_to_usbdev(interface);
    ignore = usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL); // AQUI
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    return 0;
}

static void usb_disconnect(struct usb_interface *interface)
{
    printk(KERN_INFO "WeatherStation: dispositivo desconectado.\n");
    if (sys_obj)
        kobject_put(sys_obj); // Remove os arquivos em /sys/kernel/weatherstation
    kfree(usb_in_buffer);     // Desaloca buffers
    kfree(usb_out_buffer);
}

static char *usb_send_cmd(char *cmd)
{
    int recv_size = 0; // Quantidade de caracteres no recv_line
    int ret, actual_size, i;
    int retries = 10;                  // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char resp_expected[MAX_RECV_LINE]; // Resposta esperada do comando
    char *resp_pos = NULL;             // Referência a posição da linha onde se encontra a mensagem retornada pelo ESP

    printk(KERN_INFO "WeatherStation: enviando comando: %s\n", cmd);
    sprintf(usb_out_buffer, "%s\n", cmd); // coloca o comando no buffer de saída

    // Envia o comando (usb_out_buffer) para a USB
    ret = usb_bulk_msg(weatherstation_device, usb_sndbulkpipe(weatherstation_device, usb_out), usb_out_buffer, strlen(usb_out_buffer), &actual_size, RESPONSE_TIMEOUT);
    if (ret)
    {
        printk(KERN_ERR "WeatherStation: erro de codigo %d ao enviar comando '%s'!\n", ret, cmd);
        return NULL;
    }

    sprintf(resp_expected, "RES %s", cmd); // Resposta esperada.

    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0)
    {
        // Lê dados da USB
        ret = usb_bulk_msg(weatherstation_device, usb_rcvbulkpipe(weatherstation_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, RESPONSE_TIMEOUT);
        if (ret)
        {
            printk(KERN_ERR "WeatherStation: erro ao ler dados da USB (tentativa %d). Codigo: %d\n", retries--, ret);
            continue;
        }

        // Para cada caractere recebido ...
        for (i = 0; i < actual_size; i++)
        {
            if (usb_in_buffer[i] == '\n')
            { // Temos uma linha completa
                recv_line[recv_size] = '\0';
                printk(KERN_INFO "WeatherStation: recebido uma linha: '%s'\n", recv_line);

                // Verifica se o início da linha recebida é igual à resposta esperada do comando enviado
                if (!strncmp(recv_line, resp_expected, strlen(resp_expected)))
                {
                    printk(KERN_INFO "WeatherStation: linha eh resposta para %s! Processando...\n", cmd);

                    // Acessa a parte da resposta que contém os valores
                    resp_pos = &recv_line[strlen(resp_expected) + 1];
                    return resp_pos;
                }
                else
                { // Não é a linha que estávamos esperando. Pega a próxima.
                    printk(KERN_INFO "WeatherStation: nao eh resposta para %s! Tentiva %d. Proxima linha...\n", cmd, retries--);
                    recv_size = 0; // Limpa a linha lida (recv_line)
                }
            }
            else
            { // É um caractere normal (sem ser nova linha), coloca no recv_line e lê o próximo caractere
                recv_line[recv_size] = usb_in_buffer[i];
                recv_size++;
            }
        }
    }

    return NULL; // Não recebi a resposta esperada do dispositivo
}

static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff)
{
    char *value = NULL;
    const char *attr_name = attr->attr.name;

    printk(KERN_INFO "WeatherStation: lendo %s ...\n", attr_name);
    value = usb_send_cmd("GET_DATA");

    if (value)
    {
        sprintf(buff, "%s\n", value); // Cria a mensagem com o valor da mensagem recebida
        return strlen(buff);
    }

    sprintf(buff, "%s\n", "");
    return 0;
}

static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count)
{
    const char *attr_name = attr->attr.name;

    printk(KERN_ALERT "WeatherStation: o arquivo %s eh apenas para leitura.\n", attr_name);
    return -EACCES;
}
